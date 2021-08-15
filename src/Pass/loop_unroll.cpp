#include "../../include/Pass/loop_unroll.h"
#include "../../include/ir_struct.h"

#include <vector>
#include <map>
#include <algorithm>
#include <queue>
#include <set>

using namespace std;

// 展开次数
const int TIME=12;

map<IRBasicBlock*,int> bb_id;
map<int,IRBasicBlock*> id_bb;

template<typename T>
bool have(T a,vector<T> vec)
{
    for(auto &v:vec)
        if(v==a)return true;
    return false;
}

template<typename T>
bool nothave(T a,vector<T> vec)
{
    for(auto &v:vec)
        if(v==a)return false;
    return true;
}

vector<IRBasicBlock*> cross(vector<IRBasicBlock*> a,vector<IRBasicBlock*> b)
{
    sort(a.begin(),a.end()),sort(b.begin(),b.end());
    vector<IRBasicBlock*> res;
    int ia=0,ib=0;
    while(ia<a.size() && ib<b.size())
    {
        if(a[ia]<b[ib])ia++;
        else if(a[ia]>b[ib])ib++;
        else
        {
            res.push_back(a[ia]);
            ia++,ib++;
        }
    }
    return res;
}

int find_max_label(IRModule* irmodule)
{
    int mx=0;
    for(auto& func:irmodule->func_list_)
    {
        for(auto& bb:func->bb_list_)
        {
            for(auto& ir:bb->ir_list_)
            {
                if(ir->op_==ir::IR::OpKind::LABEL)
                {
                    string tmp=ir->opn1_.name_;
                    if(*tmp.begin()=='.')
                    {
                        for(int i=0;i<6;i++)tmp.erase(tmp.begin());
                        if(*tmp.begin()=='-')continue; // 说明是循环不变量外提写的
                        mx=max(mx,stoi(tmp)); // find_max_label在add_scope_id前
                    }
                }
            }
        }
    }
    return mx;
}

int find_max_temp(IRModule* irmodule)
{
    int mx=0;
    for(auto& func:irmodule->func_list_)
    {
        for(auto& bb:func->bb_list_)
        {
            for(auto& ir:bb->ir_list_)
            {
                vector<ir::Opn*> tmp;
                tmp.push_back(&ir->opn1_);
                tmp.push_back(&ir->opn2_);
                tmp.push_back(&ir->res_);
                for(auto opn:tmp)
                {
                    if(opn->type_==ir::Opn::Type::Var && find(opn->name_.begin(),opn->name_.end(),'-')!=opn->name_.end())
                    {
                        string name=opn->name_;
                        for(int i=0;i<=4;i++)name.erase(name.begin());
                        mx=max(mx,stoi(name));
                    }
                }
            }
        }
    }
    return mx;
}

void add_scope_id(IRFunction* func)
{
    for(auto& bb:func->bb_list_)
    {
        for(auto& ir:bb->ir_list_)
        {
            vector<ir::Opn*> tmp;
            tmp.push_back(&ir->opn1_),tmp.push_back(&ir->opn2_),tmp.push_back(&ir->res_);
            for(auto& opn:tmp)
            {
                if(opn->type_==ir::Opn::Type::Var)
                    opn->name_+="_#"+to_string(opn->scope_id_);
            }
        }
    }
}

void del_scope_id(IRFunction* func)
{
    for(auto& bb:func->bb_list_)
    {
        for(auto& ir:bb->ir_list_)
        {
            vector<ir::Opn*> tmp;
            tmp.push_back(&ir->opn1_),tmp.push_back(&ir->opn2_),tmp.push_back(&ir->res_);
            for(auto& opn:tmp)
            {
                if(opn->type_==ir::Opn::Type::Var)
                {
                    while(opn->name_[opn->name_.size()-1]!='_')opn->name_.pop_back();
                    opn->name_.pop_back();
                }
            }
        }
    }
}

// 用来找回边，如果bb_list为空会RE
vector<pair<IRBasicBlock*,IRBasicBlock*> > find_back(vector<IRBasicBlock*> bb_list)
{
    // 计算必经节点集
    map<IRBasicBlock*,vector<IRBasicBlock*> > dom;
    dom[bb_list[0]]={bb_list[0]};
    for(int i=1;i<bb_list.size();i++)
    {
        for(int j=0;j<bb_list.size();j++)
            dom[bb_list[i]].push_back(bb_list[j]);
    }
    bool flag=true;
    while(flag)
    {
        flag=false;
        for(auto& bb:bb_list)
        {
            int last=dom[bb].size();
            if(bb->pred_.size()==0)dom[bb].clear();
            for(auto& pred:bb->pred_)dom[bb]=cross(dom[bb],dom[pred]);
            if(bb!=bb_list[0])dom[bb].push_back(bb);
            flag|=(last!=dom[bb].size());
        }
    }
    
    // 计算回边
    vector<pair<IRBasicBlock*,IRBasicBlock*> > back;
    for(auto& bb:bb_list)
    {
        for(auto& suc:bb->succ_)
        {
            if(have(suc,dom[bb]))
                back.push_back(make_pair(bb,suc));
        }
    }
    return back;
}

vector<IRBasicBlock*> find_loop(pair<IRBasicBlock*,IRBasicBlock*> edge)
{
    auto& u=edge.first,v=edge.second;
    vector<IRBasicBlock*> res;
    res.push_back(v);
    if(u==v)return res;
    queue<IRBasicBlock*> q;
    q.push(u);
    while(!q.empty())
    {
        auto x=q.front();
        q.pop();
        if(have(x,res))continue;
        res.push_back(x);
        for(auto& pre:x->pred_)q.push(pre);
    }
    return res;
}

// continue、break、多重循环我觉得不能有，ifelse老师说不能有，所以循环体就没有任何跳转了，循环
// 只能有2个基本块
bool check1(vector<IRBasicBlock*> loop)
{
    return loop.size()==2;
}

bool have1(ir::Opn* var,vector<ir::Opn*> bel)
{
    for(auto& b:bel)
        if(b->name_==var->name_)return true;
    return false;
}

vector<ir::Opn*> find_basicvar(ir::Opn* var,IRBasicBlock* bb)
{
    vector<ir::Opn*> basic;
    if(var->type_==ir::Opn::Type::Imm)return basic;
    basic.push_back(var);
    vector<ir::IR*> rev(bb->ir_list_);
    reverse(rev.begin(),rev.end());
    for(auto& ir:rev)
    {
        if(ir->op_==ir::IR::OpKind::LABEL ||
           ir->op_==ir::IR::OpKind::PARAM ||
           ir->op_==ir::IR::OpKind::CALL ||
           ir->op_==ir::IR::OpKind::RET ||
           ir->op_==ir::IR::OpKind::GOTO)
            continue;
        auto res=&ir->res_;
        if(have1(res,basic))
            basic.push_back(&ir->opn1_),basic.push_back(&ir->opn2_);
    }
    basic.erase(basic.begin());
    auto it=basic.begin();
    while(it!=basic.end())
    {
        string name=(*it)->name_;
        if(count(name.begin(),name.end(),'-')==1)it=basic.erase(it); // 删除临时变量，临时变量形如temp-x
        else it++;
    }
    return basic;
}

void LoopUnroll::Run()
{
    int CNT=0;

    IRModule* irmodule=dynamic_cast<IRModule*>(*m_);

    int label=find_max_label(irmodule);
    int temp=find_max_temp(irmodule);

    for(int func_id=0;func_id<irmodule->func_list_.size();func_id++)
    {

        auto& func=irmodule->func_list_[func_id];
        auto& bb_list=func->bb_list_;

        // 这次写全程用bb指针，但下面这俩是用来调试的
        bb_id.clear(),id_bb.clear();
        for(int i=0;i<bb_list.size();i++)
            bb_id[bb_list[i]]=i,id_bb[i]=bb_list[i];

        // cout<<"输出当前函数IR:"<<func->name_<<endl;
        // for(auto& bb:bb_list)
        // {
        //     cout<<"B"<<bb_id[bb]<<":"<<endl;
        //     for(auto& ir:bb->ir_list_)ir->PrintIR();
        // }
        // cout<<"输出基本块构成的图:"<<endl;
        // for(auto& bb:bb_list)
        // {
        //     cout<<bb_id[bb]<<":";
        //     for(auto& suc:bb->succ_)cout<<bb_id[suc]<<" ";
        //     cout<<endl;
        // }

        add_scope_id(func);

        // cout<<"输出当前函数IR(添加作用域后):"<<func->name_<<endl;
        // for(auto& bb:bb_list)
        // {
        //     cout<<"B"<<bb_id[bb]<<":"<<endl;
        //     for(auto& ir:bb->ir_list_)ir->PrintIR();
        // }

        // 即使后续添加了新的基本块，回边也不会变，所以可以先求出所有回边，然后每次遍历一个回边
        vector<pair<IRBasicBlock*,IRBasicBlock*> > back=find_back(bb_list);
        
        // cout<<"输出所有回边:"<<endl;
        // for(auto& edge:back)cout<<bb_id[edge.first]<<"->"<<bb_id[edge.second]<<" ";
        // cout<<endl;

        for(auto& edge:back)
        {
            vector<IRBasicBlock*> loop=find_loop(edge);

            // scope_id无法得知，但我觉得每个循环不可能没有temp临时变量，所以找个temp用它的scope_id
            int scope_id=-1;
            for(auto bb:loop)
            {
                for(auto ir:bb->ir_list_)
                {
                    vector<ir::Opn*> tmp;
                    tmp.push_back(&ir->opn1_),tmp.push_back(&ir->opn2_),tmp.push_back(&ir->res_);
                    for(auto opn:tmp)
                    {
                        if(opn->type_==ir::Opn::Type::Var && opn->name_.substr(0,4)=="temp")
                        {
                            scope_id=opn->scope_id_;
                            break;
                        }
                    }
                    if(scope_id!=-1)break;
                }
                if(scope_id!=-1)break;
            }

            // cout<<"输出当前遍历的循环:"<<endl;
            // for(auto& bb:loop)cout<<bb_id[bb]<<" ";
            // cout<<endl;

            // 限制超级大，loop只能有2块(因为有break、continue的不行，有ifelse的老师说不行，有
            // 嵌套循环的也不行)，那loop[0]就是循环条件，loop[1]就是循环体
            
            if(loop.size()!=2)continue; // 正好也限制了条件里调函数

            auto& cond_ir=loop[0]->ir_list_[loop[0]->ir_list_.size()-1];
            if(cond_ir->op_!=ir::IR::OpKind::JLT && cond_ir->op_!=ir::IR::OpKind::JLE
            && cond_ir->op_!=ir::IR::OpKind::JGT && cond_ir->op_!=ir::IR::OpKind::JGE)
                continue; //只要不是< <= > >=就不行

            auto op1=&cond_ir->opn1_,op2=&cond_ir->opn2_;
            if( (op1->type_!=ir::Opn::Type::Var && op1->type_!=ir::Opn::Type::Imm) ||
                (op2->type_!=ir::Opn::Type::Var && op2->type_!=ir::Opn::Type::Imm))
                continue; // 涉及到数组就不行

            vector<ir::Opn*> basic1=find_basicvar(op1,loop[0]),basic2=find_basicvar(op2,loop[0]);
            bool flag=true;
            for(auto basic:basic1)flag&=(basic->type_!=ir::Opn::Type::Array); // 只要basic1和2里有1个涉及到数组就不优化
            for(auto basic:basic2)flag&=(basic->type_!=ir::Opn::Type::Array);
            if(!flag)continue;
            
            // cout<<"临时"<<endl;
            // cout<<op1->name_<<" "<<op2->name_<<endl;
            // cout<<(op1->type_!=ir::Opn::Type::Var)<<" "<<(op1->type_!=ir::Opn::Type::Imm)<<" "
            // <<(op2->type_!=ir::Opn::Type::Var)<<" "<<(op2->type_!=ir::Opn::Type::Imm)<<endl;

            vector<ir::Opn*> change1=find_basicvar(op1,loop[1]),change2=find_basicvar(op2,loop[1]);
            
            // cout<<"输出找到的变量:"<<endl;
            // cout<<op1->name_<<" "<<op2->name_<<endl;
            // for(auto var:change1)cout<<var->name_<<" ";
            // cout<<endl;
            // for(auto var:change2)cout<<var->name_<<" ";
            // cout<<endl;

            if(!(change1.size()==2 && change2.size()==0) && !(change1.size()==0 && change2.size()==2))continue; // 这里后续可以改，这里限制死了，i只能是i=i op c,n不能有定值

            if(change1.size()==0)swap(change1,change2),swap(op1,op2);

            ir::IR* cal_ir;
            for(int i=0;i<loop[1]->ir_list_.size();i++)
            {
                if(loop[1]->ir_list_[i]->res_.name_==op1->name_)
                {
                    if(i && loop[1]->ir_list_[i-1]->opn1_.name_==change1[0]->name_ && loop[1]->ir_list_[i-1]->opn2_.name_==change1[1]->name_)
                        cal_ir=loop[1]->ir_list_[i-1];
                }
            }
            if(cal_ir==nullptr)continue;

            if(cal_ir->op_!=ir::IR::OpKind::ADD && cal_ir->op_!=ir::IR::OpKind::SUB)continue;

            // cout<<"输出找到的那句IR:"<<endl;
            // cal_ir->PrintIR();

            // 循环体变2倍。不新建基本块而是直接在旧基本块里把除了最后一句goto loop[0].begin()以外全部复制一遍
            // IRBasicBlock* new_body=new IRBasicBlock();
            // new_body->func_=func;
            // int num=bb_id.size();
            // bb_id[new_body]=num,id_bb[num]=new_body;
            // // loop[1]的ir复制过来
            // for(auto ir:loop[1]->ir_list_)
            // {
            //     ir::IR* new_ir=new ir::IR(ir->op_,ir->opn1_,ir->opn2_,ir->res_);
            //     new_body->ir_list_.push_back(new_ir);
            // }
            // // new_body连入图中
            // loop[1]->succ_.pop_back(),loop[0]->pred_.erase(find(loop[0]->pred_.begin(),loop[0]->pred_.end(),loop[1]));
            // loop[1]->succ_.push_back(new_body),new_body->pred_.push_back(loop[1]);
            // new_body->succ_.push_back(loop[0]),loop[0]->pred_.push_back(new_body);
            // // 新的label，new_body弄进去，loop[1]弄进去
            // ir::IR* new_ir=new ir::IR(ir::IR::OpKind::LABEL,*(new ir::Opn(ir::Opn::Type::Label,".label"+to_string(++label),scope_id)));
            // new_body->ir_list_.insert(new_body->ir_list_.begin(),new_ir);
            // loop[1]->ir_list_[loop[1]->ir_list_.size()-1]->opn1_.name_=".label"+to_string(label);

            // cout<<"输出新循环体:"<<endl;
            // for(auto& ir:new_body->ir_list_)ir->PrintIR();
            // cout<<"输出加入新循环体后的图:"<<endl;
            // for(auto& bb:bb_list)
            // {
            //     cout<<bb_id[bb]<<":";
            //     for(auto& suc:bb->succ_)cout<<bb_id[suc]<<" ";
            //     cout<<endl;
            // }
            // cout<<bb_id[new_body]<<":";
            // for(auto& suc:new_body->succ_)cout<<bb_id[suc]<<" ";
            // cout<<endl;
            
            // i+c的那个c
            ir::Opn* c;
            if(cal_ir->opn1_.name_==op1->name_)c=&cal_ir->opn2_;
            else c=&cal_ir->opn1_;

            if(op2->type_==ir::Opn::Type::Imm && c->type_==ir::Opn::Type::Imm &&
               (long long)op2->imm_num_+(TIME-1)*(long long) c->imm_num_ >2147483647ll )
                continue;

            // cout<<"输出i+c的那个c:"<<endl;
            // cout<<c->name_<<endl;

            // 调了顺序，原本该给loop[0]插入n±c，但因为new_loop要旧的loop[0]，所以下面先创建
            // new_loop并复制，然后再给loop[0]插入n±c

            vector<IRBasicBlock*> new_loop;
            new_loop.push_back(new IRBasicBlock()),new_loop.push_back(new IRBasicBlock());
            new_loop[0]->func_=new_loop[1]->func_=func;
            int num=bb_id.size();
            bb_id[new_loop[0]]=num,id_bb[num]=new_loop[0];
            ++num;
            bb_id[new_loop[1]]=num,id_bb[num]=new_loop[1];
            for(int i=0;i<2;i++)
            {
                for(auto ir:loop[i]->ir_list_)
                {
                    vector<ir::Opn> tmp;
                    tmp.push_back(ir->opn1_),tmp.push_back(ir->opn2_),tmp.push_back(ir->res_);
                    for(auto& opn:tmp)
                    {
                        if(opn.type_==ir::Opn::Type::Array)
                        {
                            ir::Opn* off=opn.offset_;
                            ir::Opn* new_off;
                            if(off->type_==ir::Opn::Type::Var)
                                new_off=new ir::Opn(off->type_,off->name_,off->scope_id_);
                            else
                                new_off=new ir::Opn(off->type_,off->imm_num_);
                            opn.offset_=new_off;
                        }
                    }
                    ir::IR* new_ir=new ir::IR(ir->op_,tmp[0],tmp[1],tmp[2]);
                    new_loop[i]->ir_list_.push_back(new_ir);
                }
            }


            ir::Opn* new_temp1=new ir::Opn(ir::Opn::Type::Var,"temp-"+to_string(++temp)+"_#"+to_string(scope_id),scope_id);
            ir::Opn* new_temp2=new ir::Opn(ir::Opn::Type::Imm,TIME-1);
            ir::IR* new_ir=new ir::IR(ir::IR::OpKind::MUL,*c,*new_temp2,*new_temp1);

            auto it=loop[0]->ir_list_.end();
            loop[0]->ir_list_.insert(--it,new_ir);

            // 如果是ADD，则i<n变i<n-TIME*c
            if(cal_ir->op_==ir::IR::OpKind::ADD)
            {
                ir::Opn* new_temp=new ir::Opn(ir::Opn::Type::Var,"temp-"+to_string(++temp)+"_#"+to_string(scope_id),scope_id);
                ir::IR* new_ir=new ir::IR(ir::IR::OpKind::SUB,*op2,*new_temp1,*new_temp);

                auto it=loop[0]->ir_list_.end();
                loop[0]->ir_list_.insert(--it,new_ir);
                if(loop[0]->ir_list_[loop[0]->ir_list_.size()-1]->opn1_.name_==op1->name_)
                    loop[0]->ir_list_[loop[0]->ir_list_.size()-1]->opn2_=*new_temp;
                else
                    loop[0]->ir_list_[loop[0]->ir_list_.size()-1]->opn1_=*new_temp;
            }
            else // SUB，i<n变i<n+TIME*c
            {
                ir::Opn* new_temp=new ir::Opn(ir::Opn::Type::Var,"temp-"+to_string(++temp)+"_#"+to_string(scope_id),scope_id);
                ir::IR* new_ir=new ir::IR(ir::IR::OpKind::ADD,*op2,*new_temp1,*new_temp);

                auto it=loop[0]->ir_list_.end();
                loop[0]->ir_list_.insert(--it,new_ir);
                if(loop[0]->ir_list_[loop[0]->ir_list_.size()-1]->opn1_.name_==op1->name_)
                    loop[0]->ir_list_[loop[0]->ir_list_.size()-1]->opn2_=*new_temp;
                else
                    loop[0]->ir_list_[loop[0]->ir_list_.size()-1]->opn1_=*new_temp;
            }

            // cout<<"输出新的循环:"<<endl;
            // for(auto& bb:new_loop)
            // {
            //     for(auto& ir:bb->ir_list_)ir->PrintIR();
            //     cout<<endl;
            // }

            new_loop[0]->ir_list_[0]->opn1_.name_=".label"+to_string(++label);
            new_loop[1]->ir_list_[new_loop[1]->ir_list_.size()-1]->opn1_.name_=".label"+to_string(label);
            loop[0]->ir_list_[loop[0]->ir_list_.size()-1]->res_.name_=".label"+to_string(label);

            if(loop[0]->succ_[1]==loop[1])swap(loop[0]->succ_[0],loop[0]->succ_[1]);
            auto next_bb=loop[0]->succ_[1];

            // cout<<"next_bb:"<<bb_id[next_bb]<<endl;
            // new_loop拼入图中
            loop[0]->succ_.pop_back(),next_bb->pred_.erase(find(next_bb->pred_.begin(),next_bb->pred_.end(),loop[0]));
            loop[0]->succ_.push_back(new_loop[0]),new_loop[0]->pred_.push_back(loop[0]);
            new_loop[0]->succ_.push_back(next_bb),next_bb->pred_.push_back(new_loop[0]);

            new_loop[0]->succ_.push_back(new_loop[1]),new_loop[1]->pred_.push_back(new_loop[0]);
            new_loop[1]->succ_.push_back(new_loop[0]),new_loop[0]->pred_.push_back(new_loop[1]);
            
            // bb_list.push_back(new_loop[0]),bb_list.push_back(new_loop[1]);
            auto insert_it=find(bb_list.begin(),bb_list.end(),loop[1]);
            insert_it++;
            bb_list.insert(insert_it,new_loop[0]);
            insert_it=find(bb_list.begin(),bb_list.end(),new_loop[0]);
            insert_it++;
            bb_list.insert(insert_it,new_loop[1]);

            int tmp=loop[1]->ir_list_.size()-1;
            for(int time=1;time<TIME;time++)
            {
                for(int i=0;i<tmp;i++)
                {
                    auto tail=loop[1]->ir_list_.end();
                    --tail;
                    auto ir=loop[1]->ir_list_[i];
                    auto op1=ir->opn1_,op2=ir->opn2_,res=ir->res_;
                    vector<ir::Opn> tmp;
                    tmp.push_back(op1),tmp.push_back(op2),tmp.push_back(res);
                    for(auto& t:tmp)
                    {
                        if(t.type_==ir::Opn::Type::Array)
                        {
                            auto off=t.offset_;
                            ir::Opn* new_off;
                            if(off->type_==ir::Opn::Type::Var)
                                new_off=new ir::Opn(off->type_,off->name_,off->scope_id_);
                            else
                                new_off=new ir::Opn(off->type_,off->imm_num_);
                            t.offset_=new_off;
                        }
                    }
                    ir::IR* new_ir=new ir::IR(ir->op_,tmp[0],tmp[1],tmp[2]);
                    loop[1]->ir_list_.insert(tail,new_ir);
                }
            }

            // IRBasicBlock* old_body=loop[1];
            // IRBasicBlock* new_body=new IRBasicBlock();
            // for(int i=0;i<TIME;i++)
            // {
            //     for(int j=0;j<old_body->ir_list_.size()-1;j++)
            //     {
            //         auto ir=loop[1]->ir_list_[j];
            //         auto op1=ir->opn1_,op2=ir->opn2_,res=ir->res_;
            //         vector<ir::Opn> tmp;
            //         tmp.push_back(op1),tmp.push_back(op2),tmp.push_back(res);
            //         for(auto& t:tmp)
            //         {
            //             if(t.type_==ir::Opn::Type::Array)
            //             {
            //                 auto off=t.offset_;
            //                 ir::Opn* new_off;
            //                 if(off->type_==ir::Opn::Type::Var)
            //                     new_off=new ir::Opn(off->type_,off->name_,off->scope_id_);
            //                 else
            //                     new_off=new ir::Opn(off->type_,off->imm_num_);
            //                 t.offset_=new_off;
            //             }
            //         }
            //         ir::IR* new_ir=new ir::IR(ir->op_,tmp[0],tmp[1],tmp[2]);
            //         new_body->ir_list_.push_back(new_ir);
            //     }
            // }
            // cout<<"输出new body IR:"<<endl;
            // for(auto ir:new_body->ir_list_)ir->PrintIR();

            // new_body->ir_list_.push_back(old_body->ir_list_[old_body->ir_list_.size()-1]);
            // old_body=new_body;

            ++CNT;

            // cout<<"再次输出新的循环:"<<endl;
            // for(auto& bb:new_loop)
            // {
            //     for(auto& ir:bb->ir_list_)ir->PrintIR();
            //     cout<<endl;
            //     cout<<"pred: ";
            //     for(auto& pre:bb->pred_)cout<<bb_id[pre]<<" ";
            //     cout<<endl;
            //     cout<<"succ: ";
            //     for(auto& suc:bb->succ_)cout<<bb_id[suc]<<" ";
            //     cout<<endl;
            // }

            // cout<<"输出处理完本循环的汇总:"<<endl;
            // cout<<"输出IR:"<<endl;
            // for(auto& bb:bb_list)
            // {
            //     cout<<"B"<<bb_id[bb]<<":"<<endl;
            //     for(auto& ir:bb->ir_list_)ir->PrintIR();
            // }
            // cout<<"输出图:"<<endl;
            // for(auto& bb:bb_list)
            // {
            //     cout<<bb_id[bb]<<":";
            //     for(auto& suc:bb->succ_)cout<<bb_id[suc]<<" ";
            //     cout<<endl;
            // }
        }
        del_scope_id(func);
    }

    // cout<<"最终汇总:"<<endl;
    // for(auto func:irmodule->func_list_)
    // {
    //     cout<<"func:"<<func->name_<<endl;
    //     for(auto bb:func->bb_list_)
    //     {
    //         cout<<"B"<<bb_id[bb]<<" ";
    //         cout<<"pred:";
    //         for(auto pre:bb->pred_)cout<<bb_id[pre]<<" ";
    //         cout<<"succ:";
    //         for(auto suc:bb->succ_)cout<<bb_id[suc]<<" ";
    //         cout<<endl;
    //         for(auto ir:bb->ir_list_)ir->PrintIR();
    //     }
    // }
    cout<<"展开了"<<CNT<<"个循环"<<endl;
}