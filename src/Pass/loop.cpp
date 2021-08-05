#include <iostream>
#include <vector>
#include <map>
#include <queue>
#include <algorithm>
#include <set>
#include <iterator>

#include "../../include/ir.h"
#include "../../include/ir_struct.h"
#include "../../include/general_struct.h"
#include "../../include/Pass/loop.h"
#include "../../include/Pass/ir_liveness_analysis.h"

using namespace std;

// TODO:能改进的地方就是：包含全局变量的不变运算看能否外提。因为中途可能调函数，所以可以选择一律不提，也可
// 以选择看有无调函数，无则提
// TODO:不知道为啥，多了个label-2 这个072

// 循环不变运算外提是在循环入口节点前面新建1个基本块，然后把不变运算都放在这个基本块里，所以
// 优化后，可能会有一些基本块按道理不该划成基本块的，甚至可能有空的基本块(不知道能不能在入口基本块的开头加)


// 求必经节点集中取交集。因为都是有序的，所以就是归并一下
vector<int> cross(vector<int> a,vector<int> b)
{
    vector<int> ret;
    int ia=0,ib=0;
    while(ia<a.size() and ib<b.size())
    {
        if(a[ia]<b[ib])ia++;
        else if(a[ia]>b[ib])ib++;
        else
        {
            ret.push_back(a[ia]);
            ia++,ib++;
        }
    }
    return ret;
}

// 二分查找x是否在points里，所以要求points有序
// 更新：不二分了，可能找错了
template<typename T>
bool have(T x,vector<T> points)
{
    // int l=0,r=points.size();
    // while(l<r)
    // {
    //     int mid=(l+r)>>1;
    //     if(points[mid]<x)l=mid+1;
    //     else r=mid;
    // }
    // return points[l]==x;
    for(int i=0;i<points.size();i++)
        if(points[i]==x)return true;
    return false;
}

// O(n)找的，points不用有序
bool not_have(int x,vector<int> points)
{
    for(int i=0;i<points.size();i++)
        if(x==points[i])return false;
    return true;
}

// 检查是否不变
bool check(ir::Opn x,vector<int> loop,map<string,vector<pair<int,int> > > def,vector<pair<int,int> > unchanegd)
{
    if(x.type_==ir::Opn::Type::Imm)return true; // 为常数则返回
    sort(loop.begin(),loop.end());
    auto defs=def[x.name_];
    for(int i=0,j=0;i<loop.size() && j<defs.size();)
    {
        if(loop[i]<defs[j].first)i++;
        else if(loop[i]>defs[j].first)j++;
        else // 发现了loop内有def，则看看那句def是否unchanged。如果是则这句也unchanged
        {
            auto now=defs[j];
            int k=0;
            while(k<unchanegd.size())
            {
                if(unchanegd[k]==now)
                    break;
                k++;
            }
            if(k==unchanegd.size())return false; // 没在unchanged里找到，说明不是unchanged，所以这句不行
            // 并且loop内可能有多句x的def。得这些def都是unchanged才行
            i++,j++;
        }
    }
    return true; // 非常数的情况下，要么是loop里没有x的def，要么是有但那些def也都是unchanged的，就说明x在loop内也是unchanged

}

// 检查变量x是否出了loop后不在活跃(复杂度巨高，应该用活跃变量分析)
bool will_not_live(pair<int,int> pos,vector<pair<int,int> > use,vector<int> suc,vector<int> loop) //use为变量x的use，suc为loop的后续节点
{
    set<int> se;
    for(int i=0;i<use.size();i++)se.insert(use[i].first);
    for(int i=0;i<suc.size();i++)
    {
        if(se.find(suc[i])!=se.end())
            return false;
    }
    return true;
}

// bool will_not_live()

// 检查变量x在loop中是否还有其他定值语句
bool check2(pair<int,int> pos,vector<int> loop,vector<pair<int,int> > def) //pos为当前的基本块，要看loop中除了pos以外其他的基本块在不在def中
{
    set<int> se;
    for(int i=0;i<def.size();i++)
    {
        if(def[i]==pos)continue;
        se.insert(def[i].first);
    }
    for(int i=0;i<loop.size();i++)
    {
        if(se.find(loop[i])!=se.end())
            return false;
    }
    return true;
}

// 检查loop中对x的使用是否都只能有本句话的定值到达。防止的是这种情况：
// x=3;while(){y=x;...x=2;}
// 这种情况你把x=2提外面了，那第一次的y=x就不对了
// 所以check就从入口开始按图遍历loop，碰到x的def就返回，碰到x的use则返回false
bool check3(int enter,vector<vector<int> > to,vector<int> out,int def,string name,map<int,IRBasicBlock*> id_bb,vector<pair<int,int> > use) // 这个入口节点直接用新建的那个基本块，好写
// enter用新建的那个空基本块作为入口，to就是图，out是出口节点，不能出去，因为有check2，所以def只会有1次，
//所以这里def是int，然后x的use情况
{
    auto def_bb=id_bb[def];
    for(int i=0;i<def_bb->ir_list_.size();i++)
    {
        if(def_bb->ir_list_[i]->opn1_.name_==name ||def_bb->ir_list_[i]->opn2_.name_==name)
            return false;
        if(def_bb->ir_list_[i]->res_.name_==name)
            break;
    }
    set<int> u,o;
    for(int i=0;i<use.size();i++)u.insert(use[i].first);
    for(int i=0;i<out.size();i++)o.insert(out[i]);
    queue<int> qqq;
    set<int> vis;
    qqq.push(enter);
    while(!qqq.empty())
    {
        int x=qqq.front();
        qqq.pop();
        for(int i=0;i<to[x].size();i++)
        {
            int y=to[x][i];
            if(y==def)continue;
            if(u.find(y)!=u.end())return false; // 发现了x的使用，则返回false
            if(o.find(y)==o.end() && vis.find(y)==vis.end()) // 既不是出口节点，也没有vis过才能入队
                qqq.push(y),vis.insert(y);
        }
    }
    return true;
}

void MXD::Run()
{
#ifdef DEBUG_LOOP_PASS
    cout<<"MXD 开始\n";
#endif

    int cntcnt=0;

    int standard=1; // 后面新建的基本块要有个新的label，从-1开始命名，label-1，label-2，...

    IRModule* irmodule=dynamic_cast<IRModule*>(*m_); //m_是Module**

#ifdef DEBUG_LOOP_PASS
    cout<<"输出IR的结构\n";
    for(int i=0;i<irmodule->func_list_.size();i++)
    {
        cout<<"f"<<i<<":\n";
        auto func=irmodule->func_list_[i];
        for(int j=0;j<func->bb_list_.size();j++)
        {
            cout<<"b"<<j<<":\n";
            auto bb=func->bb_list_[j];
            for(int k=0;k<bb->ir_list_.size();k++)bb->ir_list_[k]->PrintIR();
        }
    }
#endif
#undef DEBUG_LOOP_PASS

    for(auto& func:irmodule->func_list_)
    {
        auto bb_list=func->bb_list_;
        
        int n=0; // 点数（基本块数），先设为0待会当cnt用
        vector<vector<int> > from,to; // 有向图，from是入边，to是出边
        map<IRBasicBlock*,int> bb_id; //给每个基本块起个id
        map<int,IRBasicBlock*> id_bb; //反过来

        for(auto bb:bb_list) // 基本块映射为0~n-1，还有反过来
        {
            bb_id[bb]=n;
            id_bb[n]=bb;
            n++;
        }

        // 解决一下作用域问题，每个变量的name变成：name+"_#"+str(scope_id)
        for(int i=0;i<bb_list.size();i++)
        {
            auto tmp=bb_list[i]->ir_list_;
            for(int j=0;j<bb_list[i]->ir_list_.size();j++)
            {
                if(bb_list[i]->ir_list_[j]->opn1_.type_==ir::Opn::Type::Var)
                    bb_list[i]->ir_list_[j]->opn1_.name_+="_#"+to_string(bb_list[i]->ir_list_[j]->opn1_.scope_id_);
                else if(bb_list[i]->ir_list_[j]->opn1_.type_==ir::Opn::Type::Array)
                {
                    bb_list[i]->ir_list_[j]->opn1_.name_+="_#"+to_string(bb_list[i]->ir_list_[j]->opn1_.scope_id_);
                    bb_list[i]->ir_list_[j]->opn1_.offset_->name_+="_#"+to_string(bb_list[i]->ir_list_[j]->opn1_.offset_->scope_id_);
                }
                if(bb_list[i]->ir_list_[j]->opn2_.type_==ir::Opn::Type::Var)
                    bb_list[i]->ir_list_[j]->opn2_.name_+="_#"+to_string(bb_list[i]->ir_list_[j]->opn2_.scope_id_);
                else if(bb_list[i]->ir_list_[j]->opn2_.type_==ir::Opn::Type::Array)
                {
                    bb_list[i]->ir_list_[j]->opn2_.name_+="_#"+to_string(bb_list[i]->ir_list_[j]->opn2_.scope_id_);
                    bb_list[i]->ir_list_[j]->opn2_.offset_->name_+="_#"+to_string(bb_list[i]->ir_list_[j]->opn2_.offset_->scope_id_);
                }
                if(bb_list[i]->ir_list_[j]->res_.type_==ir::Opn::Type::Var)
                    bb_list[i]->ir_list_[j]->res_.name_+="_#"+to_string(bb_list[i]->ir_list_[j]->res_.scope_id_);
                else if(bb_list[i]->ir_list_[j]->res_.type_==ir::Opn::Type::Array)
                {
                    bb_list[i]->ir_list_[j]->res_.name_+="_#"+to_string(bb_list[i]->ir_list_[j]->res_.scope_id_);
                    bb_list[i]->ir_list_[j]->res_.offset_->name_+="_#"+to_string(bb_list[i]->ir_list_[j]->res_.offset_->scope_id_);
                }
            }
        }


#ifdef DEBUG_LOOP_PASS
        cout<<"输出处理后的IR:\n";
        for(int i=0;i<bb_list.size();i++)
        {
            cout<<"b"<<i<<":\n";
            for(int j=0;j<bb_list[i]->ir_list_.size();j++)
                bb_list[i]->ir_list_[j]->PrintIR();
        }
#endif
#undef DEBUG_LOOP_PASS

        from.resize(n),to.resize(n);
        for(auto bb:bb_list) // 构造图
        {
            int u=bb_id[bb];
            for(auto pre:bb->pred_)
            {
                int v=bb_id[pre];
                from[u].push_back(v);
            }
            for(auto suc:bb->succ_)
            {
                int v=bb_id[suc];
                to[u].push_back(v);
            }
        }

#ifdef DEBUG_LOOP_PASS
        cout<<"输出构造的图：\n";
        for(int i=0;i<n;i++) // 输出构造的图
        {
            cout<<i<<':';
            for(int j=0;j<to[i].size();j++)cout<<to[i][j]<<' ';
            cout<<endl;
        }
#endif
#undef DEBUG_LOOP_PASS

        vector<vector<int> > dom(n); // 每个节点会有1个必经节点集
        // 初始化，0只有0，其余都是0~(n-1)
        dom[0].push_back(0);
        for(int i=1;i<n;i++)
            for(int j=0;j<n;j++)
                dom[i].push_back(j);
        bool flag=true;
        int cnt=0;
        while(flag && (++cnt)<=3)
        {
            flag=false;
            for(int i=0;i<n;i++)
            {
                int cnt=dom[i].size();
                for(int j=0;j<from[i].size();j++)dom[i]=cross(dom[i],dom[from[i][j]]);
                if(i)dom[i].push_back(i); // 这里是因为，如果是0的话，dom集本来为{0}，不特判的话会变成{0,0}
                if(cnt!=dom[i].size())flag=true;
            }
        }

#ifdef DEBUG_LOOP_PASS
        cout<<"输出每个基本块的必经节点集:\n";
        for(int i=0;i<n;i++)
        {
            cout<<i<<':';
            for(int j=0;j<dom[i].size();j++)cout<<dom[i][j]<<' ';
            cout<<endl;
        }
#endif

        vector<pair<int,int> > back;
        for(int i=0;i<n;i++)
        {
            for(int j=0;j<to[i].size();j++)
            {
                if(have(to[i][j],dom[i]))
                    back.push_back({i,to[i][j]});
            }
        }

#ifdef DEBUG_LOOP_PASS
        cout<<"输出所有回边:\n";
        for(int i=0;i<back.size();i++)cout<<back[i].first<<' '<<back[i].second<<'\n';
#endif
#undef DEBUG_LOOP_PASS

        vector<vector<int> > loops;
        vector<bool> vis(n,false);
        for(int i=0;i<back.size();i++)
        {
            vector<int> loop;
            int u=back[i].first,v=back[i].second;
            if(u==v)
                loop.push_back(u);
            else
            {
                loop.push_back(u),loop.push_back(v); // 入口节点放在下标为1这里
                vis[u]=vis[v]=true;
                queue<int> qqq;
                qqq.push(u);
                int round=0;
                while(qqq.size())
                {
                    int x=qqq.front();
                    qqq.pop();
                    for(int j=0;j<from[x].size();j++)
                    {
                        int y=from[x][j];
                        if(!vis[y])
                        {
                            qqq.push(y),vis[y]=true;
                            loop.push_back(y);
                        }
                    }
                }
                for(int j=0;j<loop.size();j++)vis[loop[j]]=false;
            }
            loops.push_back(loop);
        }

#ifdef DEBUG_LOOP_PASS
        cout<<"输出所有循环:\n";
        for(int i=0;i<loops.size();i++)
        {
            for(int j=0;j<loops[i].size();j++)
                cout<<loops[i][j]<<' ';
            cout<<'\n';
        }
#endif
#undef DEBUG_LOOP_PASS

        // 先不管数组
        // 先对整个函数计算每个变量的def、use，然后就可以找出每个循环中不变的运算了
        map<string,vector<pair<int,int> > > def,use;
        for(int i=0;i<n;i++)
        {
            auto bb=id_bb[i];
            auto ir_list=bb->ir_list_;
            for(int j=0;j<ir_list.size();j++)
            {
                if(ir_list[j]->opn1_.type_==ir::Opn::Type::Var)
                    use[ir_list[j]->opn1_.name_].push_back(make_pair(i,j));
                if(ir_list[j]->opn2_.type_==ir::Opn::Type::Var)
                    use[ir_list[j]->opn2_.name_].push_back(make_pair(i,j));
                if(ir_list[j]->res_.type_==ir::Opn::Type::Var)
                    def[ir_list[j]->res_.name_].push_back(make_pair(i,j));
            }
        }

        // 找出不变运算
        for(auto loop:loops)
        {

#ifdef DEBUG_LOOP_PASS
            cout<<"处理循环:";
            for(int i=0;i<loop.size();i++)cout<<loop[i]<<' ';
            cout<<endl;
#endif
#undef DEBUG_LOOP_PASS
            vector<pair<int,int> > unchanged;
            map<pair<int,int>,bool> vis;
            int last=-1;
            while(last!=unchanged.size())
            {
                last=unchanged.size();
                for(int i=0;i<loop.size();i++)
                {
                    auto bb=id_bb[loop[i]];
                    auto ir_list=bb->ir_list_;
                    for(int j=0;j<ir_list.size();j++)
                    {
                        if(ir_list[j]->op_==ir::IR::OpKind::ADD ||
                           ir_list[j]->op_==ir::IR::OpKind::SUB ||
                           ir_list[j]->op_==ir::IR::OpKind::MUL ||
                           ir_list[j]->op_==ir::IR::OpKind::DIV ||
                           ir_list[j]->op_==ir::IR::OpKind::MOD ||
                           ir_list[j]->op_==ir::IR::OpKind::NOT ||
                           ir_list[j]->op_==ir::IR::OpKind::NEG ||
                           ir_list[j]->op_==ir::IR::OpKind::ASSIGN) // 必须得是计算或者赋值
                        {
                            if(vis.find(make_pair(loop[i],j))!=vis.end())continue;
                            // 下面这个是判断opn1、opn2是否能是不变的
                            // 不变的要求为：要么为常数；要么是变量的时候，定值点在loop外面或定值点也被标记为不变了
                            // 并且要求每个操作数不能是全局变量

                            auto op1=ir_list[j]->opn1_,op2=ir_list[j]->opn2_,res=ir_list[j]->res_;
                            bool flag=true;
                            flag&=op1.scope_id_!=0,flag&=op2.scope_id_!=0,flag&=res.scope_id_!=0;
                            if(res.type_==ir::Opn::Type::Array)
                                flag&=check(*res.offset_,loop,def,unchanged);
                            if(op1.type_==ir::Opn::Type::Array)
                                flag&=check(*op1.offset_,loop,def,unchanged);
                            else
                                flag&=check(op1,loop,def,unchanged);
                            if(op2.type_==ir::Opn::Type::Array)
                                flag&=check(*op2.offset_,loop,def,unchanged);
                            else
                                flag&=check(op2,loop,def,unchanged);
                            if(flag)
                            {
                                unchanged.push_back(make_pair(loop[i],j));
                                vis[make_pair(loop[i],j)]=true;
                            }
                        }

                    }
                }
                break;
            }

#ifdef DEBUG_LOOP_PASS
            //TODO:后续测一下递归地不变运算能不能识别出来
            cout<<"输出不变运算:\n";
            for(int i=0;i<unchanged.size();i++)cout<<'('<<unchanged[i].first<<','<<unchanged[i].second<<")\n";
            cout<<endl;
#endif
#undef DEBUG_LOOP_PASS

            // 对每个不变语句看是否能外提，能的话直接外提了

            vector<int> out; // 出口节点
            for(int i=0;i<loop.size();i++)
            {
                int u=loop[i];
                for(int j=0;j<to[u].size();j++)
                {
                    int v=to[u][j];
                    if(not_have(v,loop))
                    {
                        out.push_back(u);
                        break;
                    }
                }
            }

#ifdef DEBUG_LOOP_PASS
            cout<<"输出出口节点:\n";
            for(int i=0;i<out.size();i++)cout<<out[i]<<' ';
            cout<<endl;
#endif

            int enter; // 入口id
            if(loop.size()==1)enter=loop[0];
            else enter=loop[1];

#ifdef DEBUG_LOOP_PASS
            cout<<"输出入口节点:\n";
            cout<<enter<<endl;
#endif

            set<int> in_loop;
            for(int i=0;i<loop.size();i++)in_loop.insert(loop[i]);

            vector<int> suc; // 求一下当前loop的后续节点，后面要用
            vector<int> vis1(n,false);
            queue<int> qqq;
            for(int i=0;i<out.size();i++)qqq.push(out[i]),vis1[out[i]]=true;
            while(!qqq.empty())
            {
                int x=qqq.front();
                qqq.pop();
                for(int i=0;i<to[x].size();i++)
                {
                    int y=to[x][i];
                    if(vis1[y])continue;
                    if(in_loop.find(y)!=in_loop.end())
                    {
                        vis1[y]=true;
                        continue;
                    }
                    qqq.push(y),vis1[y]=true,suc.push_back(y);
                }
            }

#ifdef DEBUG_LOOP_PASS
            cout<<"输出后续节点:\n";
            for(int i=0;i<suc.size();i++)cout<<suc[i]<<' ';
            cout<<endl;
#endif

            vector<int> must_out; // 求出口节点的必经节点集
            for(int i=0;i<loop.size();i++)must_out.push_back(loop[i]); // 待会求交集，这里初始化为loop里所有点
            sort(must_out.begin(),must_out.end()); // 排序是因为求交集按2个都有序求的

            for(int i=0;i<out.size();i++)must_out=cross(must_out,dom[out[i]]);

#ifdef DEBUG_LOOP_PASS
            cout<<"输出出口节点的必经节点集:\n";
            for(int i=0;i<must_out.size();i++)cout<<must_out[i]<<' ';
            cout<<endl;
#endif

            IRBasicBlock* unchanged_bb=new IRBasicBlock();

            unchanged_bb->func_=func;

           // 把新基本块插入图中，loop以外的点与loop入口相连的边改为连到新基本块，新基本块连到loop入口
           // 我决定先把不变运算放进新基本块，再把他插入图中

            sort(unchanged.begin(),unchanged.end()); // 这里要排序，因为定值可能是递归地发现是定值，所以unchanged不一定有序
            /*
            2个都是定值的、传递着是定值的、
            */
            for(int i=0;i<unchanged.size();i++)
            {
                ir::Opn res=id_bb[unchanged[i].first]->ir_list_[unchanged[i].second]->res_;
                ir::Opn op1=id_bb[unchanged[i].first]->ir_list_[unchanged[i].second]->opn1_;
                ir::Opn op2=id_bb[unchanged[i].first]->ir_list_[unchanged[i].second]->opn2_;

// cout<<"遍历unchanged"<<unchanged[i].first<<' '<<unchanged[i].second<<endl;
// cout<<"临时:";
// cout<<res.name_<<' '<<op1.name_<<' '<<op2.name_<<endl;
// cout<<res.scope_id_<<' '<<op1.scope_id_<<' '<<op2.scope_id_<<' ';
// cout<<have(unchanged[i].first,must_out)<<' '<<
// will_not_live(unchanged[i],use[res.name_],suc,loop)<<' '<<
// check2(unchanged[i],loop,def[res.name_])<<' '<<
// check3(enter,to,out,unchanged[i].first,res.name_,id_bb,use[res.name_])<<endl;

                // 再加个条件：全局的不能外提。遇到全局可以跳过，也可以扫一下看有没有调函数。但这里就跳过了
                if(res.scope_id_==0 || op1.scope_id_==0 || op2.scope_id_==0)continue;

                // 要能外提，需要满足以下3个条件：
                // 1.要么是当前基本块为loop中所有出口节点的必经节点，也就是当前基本块在must_out里，要么
                // 是当前变量后续不再活跃，即will_not_live(注意，这里那个网站说错了，网站说的是出了
                // loop不再活跃，但loop内可能还会用，直接提出去就错了)
                // 2.res在loop中不再有其他定值，即check2(解释：本句指令前后都不能有res的定值语句)
                // 3.loop中其他对于res的使用都只会从本句指令到达，即check3(从loop中的入口往后走，看能不能不经过本基本块到达res的使用)
                // if((have(unchanged[i].first,must_out) || will_not_live(unchanged[i],use[res.name_],suc,loop)) &&
                //    check2(unchanged[i],loop,def[res.name_]) &&
                //    check3(enter,to,out,unchanged[i].first,res.name_,id_bb,use[res.name_]))
                if((have(unchanged[i].first,must_out)) &&
                   check2(unchanged[i],loop,def[res.name_]) &&
                   check3(enter,to,out,unchanged[i].first,res.name_,id_bb,use[res.name_]))
                {
                    ++cntcnt;
                    unchanged_bb->ir_list_.push_back(id_bb[unchanged[i].first]->ir_list_[unchanged[i].second]);
                    // id_bb[unchanged[i].first]->ir_list_[unchanged[i].second]->op_=ir::IR::OpKind::VOID;
                    auto new_it=id_bb[unchanged[i].first]->ir_list_.begin();
                    for(int k=0;k<unchanged[i].second;k++)new_it++;
                    id_bb[unchanged[i].first]->ir_list_.erase(new_it);
                    int j=i+1;
                    while(j<unchanged.size() && unchanged[j].first==unchanged[i].first)
                    {
                        unchanged[j].second--;
                        j++;
                    }
                    def.clear();
                    use.clear();
                    for(int i=0;i<n;i++)
                    {
                        auto bb=id_bb[i];
                        auto ir_list=bb->ir_list_;
                        for(int j=0;j<ir_list.size();j++)
                        {
                            if(ir_list[j]->opn1_.type_==ir::Opn::Type::Var)
                                use[ir_list[j]->opn1_.name_].push_back(make_pair(i,j));
                            if(ir_list[j]->opn2_.type_==ir::Opn::Type::Var)
                                use[ir_list[j]->opn2_.name_].push_back(make_pair(i,j));
                            if(ir_list[j]->res_.type_==ir::Opn::Type::Var)
                                def[ir_list[j]->res_.name_].push_back(make_pair(i,j));
                        }
                    }
                }
            }

#ifdef DEBUG_LOOP_PASS
            cout<<"外提后:\n";
            for(int i=0;i<loop.size();i++)
            {
                auto bb=id_bb[loop[i]];
                cout<<"b"<<loop[i]<<"\n";
                for(int j=0;j<bb->ir_list_.size();j++)bb->ir_list_[j]->PrintIR();
            }

            cout<<"新基本块:\n";
            for(int i=0;i<unchanged_bb->ir_list_.size();i++)unchanged_bb->ir_list_[i]->PrintIR();
#endif

            // 前面有个set<int> in_loop，可以用于判断点是否在loop内
            vector<int> ps; // 循环外指向入口的点们
            for(int i=0;i<from[enter].size();i++)
            {
                int y=from[enter][i];
                if(y<=n-standard && y>=enter)continue;
                if(in_loop.find(y)==in_loop.end())
                    ps.push_back(y);
            }

#ifdef DEBUG_LOOP_PASS
            cout<<"循环外指向入口的点们:\n";
            for(int i=0;i<ps.size();i++)cout<<ps[i]<<' ';
            cout<<endl;
#endif

            string old_label=id_bb[enter]->ir_list_[0]->opn1_.name_;
            string new_label=".label."+to_string(standard++);
            ir::IR* new_ir=new ir::IR();
            new_ir->op_=ir::IR::OpKind::LABEL,new_ir->opn1_.name_=new_label,new_ir->opn1_.type_=ir::Opn::Type::Label;
            unchanged_bb->ir_list_.insert(unchanged_bb->ir_list_.begin(),new_ir);
            for(int i=0;i<ps.size();i++)
            {
                auto bb=id_bb[ps[i]];
                for(int j=0;j<bb->ir_list_.size();j++)
                {
                    if(bb->ir_list_[j]->opn1_.type_==ir::Opn::Type::Label && bb->ir_list_[j]->opn1_.name_==old_label)
                        bb->ir_list_[j]->opn1_.name_=new_label;
                    if(bb->ir_list_[j]->res_.type_==ir::Opn::Type::Label && bb->ir_list_[j]->res_.name_==old_label)
                        bb->ir_list_[j]->res_.name_=new_label;
                }
            }
            
            int ttt=0; // 因为插乱了已经，所以入口节点在bb_list中的编号不一定是enter了，要找一下
            while(func->bb_list_[ttt]!=id_bb[enter])ttt++;
            auto enter_it=func->bb_list_.begin();
            for(int i=0;i<ttt;i++)enter_it++;
            func->bb_list_.insert(enter_it,unchanged_bb);
            bb_id[unchanged_bb]=n,id_bb[n]=unchanged_bb;
            n++;
            vector<int> emp1,emp2;
            to.push_back(emp1),from.push_back(emp2);
            for(int i=0;i<ps.size();i++)
            {
                int x=ps[i];
                for(auto it=to[x].begin();it!=to[x].end();it++) // 图上x到enter的删除
                {
                    if(*it==enter)
                    {
                        to[x].erase(it);
                        break;
                    }
                }
                for(auto it=from[enter].begin();it!=from[enter].end();it++) // 图上from的x到enter删除
                {
                    if(*it==x)
                    {
                        from[enter].erase(it);
                        break;
                    }
                }
                for(auto it=id_bb[x]->succ_.begin();it!=id_bb[x]->succ_.end();it++) // 基本块上x到enter删除
                {
                    if(*it==id_bb[enter])
                    {
                        id_bb[x]->succ_.erase(it);
                        break;
                    }
                }
                for(auto it=id_bb[enter]->pred_.begin();it!=id_bb[enter]->pred_.end();it++) // 基本块上pred的x到enter删除
                {
                    if(*it==id_bb[x])
                    {
                        id_bb[enter]->pred_.erase(it);
                        break;
                    }
                }
                id_bb[x]->succ_.push_back(unchanged_bb);
                id_bb[n-1]->pred_.push_back(id_bb[x]);

                to[x].push_back(n-1);
                from[n-1].push_back(x);
            }
            unchanged_bb->succ_.push_back(id_bb[enter]);
            id_bb[enter]->pred_.push_back(unchanged_bb);
            from[enter].push_back(n-1);
            to[n-1].push_back(enter);

        #ifdef DEBUG_LOOP_PASS
                cout<<"输出循环不变运算外提后的图:\n";
                for(int i=0;i<n;i++)
                {
                    cout<<i<<":";
                    for(int j=0;j<to[i].size();j++)cout<<to[i][j]<<' ';
                    cout<<endl;
                }
                
                cout<<"输出循环不变运算外提后的基本块和前驱后继:\n";
                for(int i=0;i<func->bb_list_.size();i++)
                {
                    cout<<"b"<<bb_id[func->bb_list_[i]]<<": 前驱:";
                    for(int j=0;j<func->bb_list_[i]->pred_.size();j++)cout<<bb_id[func->bb_list_[i]->pred_[j]]<<' ';
                    cout<<"后继:";
                    for(int j=0;j<func->bb_list_[i]->succ_.size();j++)cout<<bb_id[func->bb_list_[i]->succ_[j]]<<' ';
                    cout<<"\n";
                    for(int j=0;j<func->bb_list_[i]->ir_list_.size();j++)func->bb_list_[i]->ir_list_[j]->PrintIR();
                }
        #endif

        }
        
        for(int i=0;i<func->bb_list_.size();i++)
        {
            for(int j=0;j<func->bb_list_[i]->ir_list_.size();j++)
            {
                if(func->bb_list_[i]->ir_list_[j]->opn1_.type_==ir::Opn::Type::Var)
                {
                    while(func->bb_list_[i]->ir_list_[j]->opn1_.name_[func->bb_list_[i]->ir_list_[j]->opn1_.name_.size()-1]!='#')func->bb_list_[i]->ir_list_[j]->opn1_.name_.pop_back();
                    func->bb_list_[i]->ir_list_[j]->opn1_.name_.pop_back(),func->bb_list_[i]->ir_list_[j]->opn1_.name_.pop_back();
                }
                else if(func->bb_list_[i]->ir_list_[j]->opn1_.type_==ir::Opn::Type::Array)
                {
                    while(func->bb_list_[i]->ir_list_[j]->opn1_.name_[func->bb_list_[i]->ir_list_[j]->opn1_.name_.size()-1]!='#')func->bb_list_[i]->ir_list_[j]->opn1_.name_.pop_back();
                    func->bb_list_[i]->ir_list_[j]->opn1_.name_.pop_back(),func->bb_list_[i]->ir_list_[j]->opn1_.name_.pop_back();
                    while(func->bb_list_[i]->ir_list_[j]->opn1_.offset_->name_[func->bb_list_[i]->ir_list_[j]->opn1_.offset_->name_.size()-1]!='#')func->bb_list_[i]->ir_list_[j]->opn1_.offset_->name_.pop_back();
                    func->bb_list_[i]->ir_list_[j]->opn1_.offset_->name_.pop_back(),func->bb_list_[i]->ir_list_[j]->opn1_.offset_->name_.pop_back();
                }
                if(func->bb_list_[i]->ir_list_[j]->opn2_.type_==ir::Opn::Type::Var)
                {
                    while(func->bb_list_[i]->ir_list_[j]->opn2_.name_[func->bb_list_[i]->ir_list_[j]->opn2_.name_.size()-1]!='#')func->bb_list_[i]->ir_list_[j]->opn2_.name_.pop_back();
                    func->bb_list_[i]->ir_list_[j]->opn2_.name_.pop_back(),func->bb_list_[i]->ir_list_[j]->opn2_.name_.pop_back();
                }
                else if(func->bb_list_[i]->ir_list_[j]->opn2_.type_==ir::Opn::Type::Array)
                {
                    while(func->bb_list_[i]->ir_list_[j]->opn2_.name_[func->bb_list_[i]->ir_list_[j]->opn2_.name_.size()-1]!='#')func->bb_list_[i]->ir_list_[j]->opn2_.name_.pop_back();
                    func->bb_list_[i]->ir_list_[j]->opn2_.name_.pop_back(),func->bb_list_[i]->ir_list_[j]->opn2_.name_.pop_back();
                    while(func->bb_list_[i]->ir_list_[j]->opn2_.offset_->name_[func->bb_list_[i]->ir_list_[j]->opn2_.offset_->name_.size()-1]!='#')func->bb_list_[i]->ir_list_[j]->opn2_.offset_->name_.pop_back();
                    func->bb_list_[i]->ir_list_[j]->opn2_.offset_->name_.pop_back(),func->bb_list_[i]->ir_list_[j]->opn2_.offset_->name_.pop_back();
                }
                if(func->bb_list_[i]->ir_list_[j]->res_.type_==ir::Opn::Type::Var)
                {
                    while(func->bb_list_[i]->ir_list_[j]->res_.name_[func->bb_list_[i]->ir_list_[j]->res_.name_.size()-1]!='#')func->bb_list_[i]->ir_list_[j]->res_.name_.pop_back();
                    func->bb_list_[i]->ir_list_[j]->res_.name_.pop_back(),func->bb_list_[i]->ir_list_[j]->res_.name_.pop_back();
                }
                else if(func->bb_list_[i]->ir_list_[j]->res_.type_==ir::Opn::Type::Array)
                {
                    while(func->bb_list_[i]->ir_list_[j]->res_.name_[func->bb_list_[i]->ir_list_[j]->res_.name_.size()-1]!='#')func->bb_list_[i]->ir_list_[j]->res_.name_.pop_back();
                    func->bb_list_[i]->ir_list_[j]->res_.name_.pop_back(),func->bb_list_[i]->ir_list_[j]->res_.name_.pop_back();
                    while(func->bb_list_[i]->ir_list_[j]->res_.offset_->name_[func->bb_list_[i]->ir_list_[j]->res_.offset_->name_.size()-1]!='#')func->bb_list_[i]->ir_list_[j]->res_.offset_->name_.pop_back();
                    func->bb_list_[i]->ir_list_[j]->res_.offset_->name_.pop_back(),func->bb_list_[i]->ir_list_[j]->res_.offset_->name_.pop_back();
                }
            }
        }
        // for(int i=0;i<func->bb_list_.size();i++)
        // {
        //     for(auto it=func->bb_list_[i]->ir_list_.begin();it!=func->bb_list_[i]->ir_list_.end();)
        //     {  
        //         if((*it)->op_==ir::IR::OpKind::VOID)
        //             func->bb_list_[i]->ir_list_.erase(it);
        //         else
        //             it++;
        //     }
        // }


#ifdef DEBUG_LOOP_PASS
        cout<<"输出处理该函数后的IR:\n";
        for(int i=0;i<func->bb_list_.size();i++)
        {
            cout<<"b"<<i<<":\n";
            for(int j=0;j<func->bb_list_[i]->ir_list_.size();j++)
                func->bb_list_[i]->ir_list_[j]->PrintIR();
        }
#endif
#undef DEBUG_LOOP_PASS
    }

#ifdef DEBUG_LOOP_PASS
    cout<<"汇总:\n";
    for(auto func:irmodule->func_list_)
    {
        cout<<"func:\n";
        int cnt=0;
        for(auto bb:func->bb_list_)
        {
            cout<<"bb"<<cnt++<<":\n";
            for(auto ir:bb->ir_list_)
                ir->PrintIR();
        }
    }
    cout<<"MXD 结束\n";
#endif
#undef DEBUG_LOOP_PASS
    cout<<"外提了指令："<<cntcnt<<"条"<<endl;
}