#include "../../include/Pass/loop_unroll.h"
#include "../../include/ir_struct.h"

#include <vector>
#include <map>
#include <algorithm>
#include <queue>
#include <set>

using namespace std;

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

int find_max_label(IRFunction* func)
{
    int mx=0;
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
    return mx;
}

int find_max_temp(IRFunction* func)
{
    int mx=0;
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
                    for(int i=0;i<4;i++)name.erase(name.begin());
                    mx=max(mx,stoi(name));
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

// // 这个类只用于find_change函数的返回值，因为返回3个不好写
// class find_change_res
// {
// public:
//     int flag;
//     ir::IR* ir;
//     ir::Opn* var;
//     find_change_res(int f,ir::IR* i,ir::Opn* v){flag=f,ir=i,var=v;}
// };

// // 对于bel里的变量，找哪些变量在loop中有定值
// // flag0表示无变化，1有1次变化，2有>1次变化，ir是真正计算的那条ir，var是哪个变量变化了
// find_change_res find_change(vector<ir::Opn*> bel,vector<IRBasicBlock*> loop)
// {
//     pair<ir::IR*,ir::Opn*> pos=make_pair(nullptr,nullptr);
//     for(auto& op:bel)
//     {
//         for(auto& bb:loop)
//         {
//             if(bb==loop[0])continue;
//             for(auto& ir:bb->ir_list_)
//             {
//                 auto res=&ir->res_;
//                 if(res->name_==op->name_)
//                 {
                    
//                     if(pos.first==0)pos=make_pair(ir,op);
//                     else return *(new find_change_res(2,nullptr,nullptr));
//                 }
//             }
//         }
//     }
//     if(pos.first)
//     {
//         ir::IR* last=nullptr;
//         for(int i=0;i<loop[1]->ir_list_.size()-1;i++)
//         {
//             if(loop[1]->ir_list_[i+1]==pos.second)
//             {
//                 if(loop[1]->ir_list_[i]->op_!=ir::IR::OpKind::ADD &&
//                    loop[1]->ir_list_[i]->op_!=ir::IR::OpKind::SUB)
//                    return *(new find_change_res(2,nullptr,nullptr));
//                 pos.second=loop[1]->ir_list_[i];
//             }
//         }
//     }
//     if(pos.first==0)return *(new find_change_res(0,nullptr,nullptr));
//     else return *(new find_change_res(1,pos.first,pos.second));
// }

// // check1检查迭代关系是否是<><=>=，2个操作数应该均为常数或Var并且不是全局变量，并且
// // 1个在loop内无定值，另1个(及其所属)在loop内只有1次定值
// find_change_res c1,c2;
// bool check2(vector<IRBasicBlock*> loop)
// {
//     auto& ir=loop[0]->ir_list_[loop[0]->ir_list_.size()-1];
//     if(ir->op_!=ir::IR::OpKind::JLT && ir->op_!=ir::IR::OpKind::JLE
//     && ir->op_!=ir::IR::OpKind::JGT && ir->op_!=ir::IR::OpKind::JGE)
//         return false; //只要不是< <= > >=就不行

//     auto op1=&ir->opn1_;
//     auto op2=&ir->opn2_;
//     if(op1->type_!=ir::Opn::Type::Imm && op1->type_!=ir::Opn::Type::Var || 
//        op2->type_==ir::Opn::Type::Imm && op2->type_==ir::Opn::Type::Var)
//         return false;
//     vector<ir::Opn*> bel1=find_basicvar(op1,loop[0]),bel2=find_basicvar(op2,loop[0]); // 保存产生了op1、op2的用户变量
    
//     cout<<"输出找到的用户变量:"<<endl;
//     cout<<op1->name_<<":";
//     for(auto var:bel1)cout<<var->name_<<" ";
//     cout<<endl;
//     cout<<op2->name_<<":";
//     for(auto var:bel2)cout<<var->name_<<" ";
//     cout<<endl;
    
//     c1=find_change(bel1,loop),c2=find_change(bel2,loop); // 在循环中找谁的变量变了
    
//     cout<<"输出查找变化的结果:"<<endl;
//     cout<<c1.flag<<" "<<c2.flag<<endl;

//     if(c1.flag==0 && c2.flag==1)swap(c1,c2);

//     return (c1.flag==0 && c2.flag==1 && c2.ir->op_==ir::IR::OpKind::ADD) || c1.flag==1 && c2.flag==0;
// }

// 只能是2个基本块，循环条件必须1个i1个n，i的变化只能是+-
// ir::Opn* op_i,op_n;
// bool check(vector<IRBasicBlock*> loop)
// {
//     if(loop.size()!=2)return false; // 正好也限制了条件里调函数

//     auto& cond_ir=loop[0]->ir_list_[loop[0]->ir_list_.size()-1];
//     if(cond_ir->op_!=ir::IR::OpKind::JLT && cond_ir->op_!=ir::IR::OpKind::JLE
//     && cond_ir->op_!=ir::IR::OpKind::JGT && cond_ir->op_!=ir::IR::OpKind::JGE)
//         return false; //只要不是< <= > >=就不行

//     auto op1=&cond_ir->opn1_,op2=&cond_ir->opn2_;
//     if(op1->type_!=ir::Opn::Type::Var && op1->type_!=ir::Opn::Type::Imm ||
//        op2->type_!=ir::Opn::Type::Var && op2->type_!=ir::Opn::Type::Imm)
//         return false; // 涉及到数组就不行

//     vector<ir::Opn*> bel1=find_basicvar(op1,loop[0]),bel2=find_basicvar(op2,loop[0]);
//     vector<ir::Opn*> change1=find_basicvar(op1,loop[1]),change2=find_basicvar(op2,loop[1]);
    
//     cout<<"输出找到的变量:"<<endl;
//     cout<<op1->name_<<" "<<op2->name_<<endl;
//     for(auto var:bel1)cout<<var->name_<<" ";
//     cout<<endl;
//     for(auto var:bel2)cout<<var->name_<<" ";
//     cout<<endl;
//     for(auto var:change1)cout<<var->name_<<" ";
//     cout<<endl;
//     for(auto var:change2)cout<<var->name_<<" ";
//     cout<<endl;

//     if(!(change1.size()==2 && change2.size()==0) && !(change1.size()==0 && change2.size()==2))return false; // 这里后续可以改，这里限制死了，i只能是i=i op c,n不能有定值

//     return true;
// }

void LoopUnroll::Run()
{
    IRModule* irmodule=dynamic_cast<IRModule*>(*m_);

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
        
        int label=find_max_label(func);
        int temp=find_max_temp(func);
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
            if(op1->type_!=ir::Opn::Type::Var && op1->type_!=ir::Opn::Type::Imm ||
            op2->type_!=ir::Opn::Type::Var && op2->type_!=ir::Opn::Type::Imm)
                continue; // 涉及到数组就不行

            vector<ir::Opn*> basic1=find_basicvar(op1,loop[0]),basic2=find_basicvar(op2,loop[0]);
            bool flag=true;
            for(auto basic:basic1)flag&=(basic->type_!=ir::Opn::Type::Array); // 只要basic1和2里有1个涉及到数组就不优化
            for(auto basic:basic2)flag&=(basic->type_!=ir::Opn::Type::Array);
            if(!flag)continue;
            
            cout<<"临时"<<endl;
            cout<<op1->name_<<" "<<op2->name_<<endl;
            cout<<(op1->type_!=ir::Opn::Type::Var)<<" "<<(op1->type_!=ir::Opn::Type::Imm)<<" "
            <<(op2->type_!=ir::Opn::Type::Var)<<" "<<(op2->type_!=ir::Opn::Type::Imm)<<endl;

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

            // cout<<"输出找到的那句IR:"<<endl;
            // cal_ir->PrintIR();

            // 前面的结果是：确认了格式可以，然后找到了i(op1)、n(op2)和i=i+c(cal_ir)
            IRBasicBlock* new_body=new IRBasicBlock();
            new_body->func_=func;
            int num=bb_id.size();
            bb_id[new_body]=num,id_bb[num]=new_body;
            // loop[1]的ir复制过来
            for(auto ir:loop[1]->ir_list_)
            {
                ir::IR* new_ir=new ir::IR(ir->op_,ir->opn1_,ir->opn2_,ir->res_);
                new_body->ir_list_.push_back(new_ir);
            }
            // new_body连入图中
            loop[1]->succ_.pop_back(),loop[0]->pred_.erase(find(loop[0]->pred_.begin(),loop[0]->pred_.end(),loop[1]));
            loop[1]->succ_.push_back(new_body),new_body->pred_.push_back(loop[1]);
            new_body->succ_.push_back(loop[0]),loop[0]->pred_.push_back(new_body);
            // 新的label，new_body弄进去，loop[1]弄进去
            ir::IR* new_ir=new ir::IR(ir::IR::OpKind::LABEL,*(new ir::Opn(ir::Opn::Type::Label,".label"+to_string(++label),scope_id)));
            new_body->ir_list_.insert(new_body->ir_list_.begin(),new_ir);
            loop[1]->ir_list_[loop[1]->ir_list_.size()-1]->opn1_.name_=".label"+to_string(label);

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

            // cout<<"输出i+c的那个c:"<<endl;
            // cout<<c->name_<<endl;

            // 如果是ADD，则i<n变i<n-c
            if(cal_ir->op_==ir::IR::OpKind::ADD)
            {
                ir::Opn* new_temp=new ir::Opn(ir::Opn::Type::Var,"temp-"+to_string(++temp)+"_#"+to_string(scope_id),scope_id);
                ir::IR* new_ir=new ir::IR(ir::IR::OpKind::SUB,*op2,*c,*new_temp);

                auto it=loop[0]->ir_list_.end();
                loop[0]->ir_list_.insert(--it,new_ir);
                if(loop[0]->ir_list_[loop[0]->ir_list_.size()-1]->opn1_.name_==op1->name_)
                    loop[0]->ir_list_[loop[0]->ir_list_.size()-1]->opn2_=*new_temp;
                else
                    loop[0]->ir_list_[loop[0]->ir_list_.size()-1]->opn1_=*new_temp;
            }
            else // SUB，i<n变i<n+c
            {
                ir::Opn* new_temp=new ir::Opn(ir::Opn::Type::Var,"temp-"+to_string(++temp)+"_#"+to_string(scope_id),scope_id);
                ir::IR* new_ir=new ir::IR(ir::IR::OpKind::ADD,*op2,*c,*new_temp);

                auto it=loop[0]->ir_list_.end();
                loop[0]->ir_list_.insert(--it,new_ir);
                if(loop[0]->ir_list_[loop[0]->ir_list_.size()-1]->opn1_.name_==op1->name_)
                    loop[0]->ir_list_[loop[0]->ir_list_.size()-1]->opn2_=*new_temp;
                else
                    loop[0]->ir_list_[loop[0]->ir_list_.size()-1]->opn1_=*new_temp;
            }

            // cout<<"输出新的循环条件:"<<endl;
            // cout<<"B"<<bb_id[loop[0]]<<":"<<endl;
            // for(auto& ir:loop[0]->ir_list_)ir->PrintIR();

            vector<IRBasicBlock*> new_loop;
            new_loop.push_back(new IRBasicBlock()),new_loop.push_back(new IRBasicBlock());
            new_loop[0]->func_=new_loop[1]->func_=func;
            num=bb_id.size();
            bb_id[new_loop[0]]=num,id_bb[num]=new_loop[0];
            ++num;
            bb_id[new_loop[1]]=num,id_bb[num]=new_loop[1];
            for(int i=0;i<2;i++)
            {
                for(auto ir:loop[i]->ir_list_)
                {
                    ir::IR* new_ir=new ir::IR(ir->op_,ir->opn1_,ir->opn2_,ir->res_);
                    new_loop[i]->ir_list_.push_back(new_ir);
                }
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

            bb_list.push_back(new_body),bb_list.push_back(new_loop[0]),bb_list.push_back(new_loop[1]);

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

    cout<<"最终汇总:"<<endl;
    for(auto func:irmodule->func_list_)
    {
        cout<<"func:"<<func->name_<<endl;
        for(auto bb:func->bb_list_)
        {
            cout<<"B"<<bb_id[bb]<<" ";
            cout<<"pred:";
            for(auto pre:bb->pred_)cout<<bb_id[pre]<<" ";
            cout<<"succ:";
            for(auto suc:bb->succ_)cout<<bb_id[suc]<<" ";
            cout<<endl;
            for(auto ir:bb->ir_list_)ir->PrintIR();
        }
    }
}