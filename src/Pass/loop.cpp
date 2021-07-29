#include <iostream>
#include <vector>
#include <map>

#include "../../include/ir_struct.h"
#include "../../include/general_struct.h"
#include "../../include/Pass/loop.h"

using namespace std;

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

bool dfs(int x,int goal,vector<int> loop)
{
    if(x==goal)return true;
    vis[x]=true;
    for(int i=0;i<from[x].size();i++)
    {
        int y=from[x][i];
        if(vis[y])continue;
        loop.push_back(y);
        if(dfs(y,loop))return;
        loop.pop_back();
    }
}

void MXD::Run()
{
    cout<<"MXD 开始\n";

    IRModule* irmodule=dynamic_cast<IRModule*>(*m_); //m_是Module**

    for(auto func:irmodule->func_list_)
    {
        auto bb_list=func->bb_list_;
        
        int n=0; // 点数（基本块数），先设为0待会当cnt用
        vector<vector<int> > from,to; // 有向图，from是入边，to是出边
        map<IRBasicBlock*,int> bb_id; //给每个基本块起个id

        for(auto bb:bb_list)bb_id[bb]=n++; // 基本块映射为0~n-1
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

        for(int i=0;i<n;i++) // 输出构造的图
        {
            cout<<i<<':';
            for(int j=0;j<to[i].size();j++)cout<<to[i][j]<<' ';
            cout<<endl;
        }

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
            cout<<"第"<<cnt<<"轮\n";
            for(int i=0;i<n;i++)
            {
                int cnt=dom[i].size();
                for(int j=0;j<from[i].size();j++)
                {
                    cout<<i<<' '<<j<<':';
                    for(int k=0;k<dom[i].size();k++)cout<<dom[i][k]<<',';
                    cout<<' ';
                    for(int k=0;k<dom[from[i][j]].size();k++)cout<<dom[from[i][j]][k]<<',';
                    cout<<' ';
                    dom[i]=cross(dom[i],dom[from[i][j]]);
                    for(int k=0;k<dom[i].size();k++)cout<<dom[i][k]<<',';
                    cout<<' ';
                    cout<<endl;
                    dom[i].push_back(i);
                }
                if(cnt!=dom[i].size())flag=true;
                cout<<i<<':';
                for(int j=0;j<dom[i].size();j++)cout<<dom[i][j]<<' ';
                cout<<endl;
            }
        }

        for(int i=0;i<n;i++)
        {
            cout<<i<<':';
            for(int j=0;j<dom[i].size();j++)cout<<dom[i][j]<<' ';
            cout<<endl;
        }
    }
    std::cout<<'\n';
    
    vector<vector<int> > back;
    back.resize(n);
    for(int i=0;i<n;i++)
    {
        for(int j=0;j<to[i].size();j++)
        {
            if(have(to[i][j],dom[i]))
                back[i].push_back(to[i][j]);
        }
    }
    
    vector<bool> vis;
    
    vis.resize(n,false);
    for(int i=0;i<n;i++)
    {
        for(int j=0;j<back[i].size();j++)
        {
            if(back[i][j]!=i)
            {
                int u=i,v=back[i][j];
                
                vector<int> loop;
                if(dfs(u,loop))
                {
                    for(int i=0;i<loop.size();i++)cout<<loop[i]<<' ';
                    cout<<endl;
                    loop.clear();
                }
            }
        }
    }
    cout<<"MXD 结束\n";
    exit(0);
}