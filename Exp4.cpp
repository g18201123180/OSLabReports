#include <iostream>
#include <cstdlib>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
using namespace std;

#define random(a, b) (rand() % (b - a) + a)
enum Replacement
{
    LRU,
    FIFO,
    OPT
};

const int maxm = 5, maxn = 10;
int r;              //置换算法
int ra[maxm][maxn]; //最多生成10个访问
int l[maxm];
long long start;
pthread_mutex_t rw; //互斥访问内存

//页表项
typedef struct Block
{
    int pageID;     //页面号
    int chunkID;    //页块号
    bool state;     //状态位
    int visited;    //访问位
    bool changed;   //修改位
    long long time; //载入内存的时刻
} Block;

//页表
typedef struct PageTable
{
    int progessID;
    Block *pages;
    int length;
} PageTable;
PageTable pagetable[maxm];

//内存,1为占用，0为未占用
//4MB/4KB=1024
int Memory[1024];

int getChunkID()
{
    for (int i = 0; i < 1024; i++)
    {
        if (!Memory[i])
            return i;
    }
    return -1;
}

//单位：毫秒
long long getTime()
{
    timeval tv;
    gettimeofday(&tv, NULL);
    long long now = tv.tv_sec * 1000 * 1000 + tv.tv_usec;
    return (now - start) / 1000;
}

void load(PageTable &pt, int id, int l, int *ra)
{
    int c = getChunkID();
    if (c != -1)
    {
        pt.pages[id].chunkID = c;
        Memory[c] = 1;
    }
    else
    {
        int t = 0, i = 0, maxval = l;
        for (; i < pt.length; i++)
        {
            if (pt.pages[i].state)
            {
                t = i;
                break;
            }
        }
        switch (r)
        {
        case LRU:
            //选择计数器最小的淘汰
            for (; i < pt.length; i++)
            {
                if (pt.pages[i].state == 1 && (pt.pages[i].visited < pt.pages[t].visited))
                    t = i;
            }
            break;
        case FIFO:
            //选择最先进入主存的
            for (; i < pt.length; i++)
            {
                if (pt.pages[i].state == 1 && (pt.pages[i].time < pt.pages[t].time))
                    t = i;
            }
            break;
        case OPT:
            //选择val最大的淘汰
            for (int j = l - 1; j >= 0; j++)
            {
                if (ra[j] == t)
                    maxval = j;
            }
            for (; i < pt.length; i++)
            {
                if (maxval == l)
                {
                    t = i;
                    break;
                }
                int val = l;
                for (int j = l - 1; j >= 0; j++)
                {
                    if (ra[j] == i)
                        val = j;
                }
                if (val > maxval)
                {
                    t = i;
                    maxval = val;
                }
            }
            break;
        default:
            break;
        }
        cout << "内存已满，置换页面" << t << endl;
        pt.pages[t].state = 0;
        pt.pages[id].chunkID = pt.pages[t].chunkID;
    }
    pt.pages[id].state = 1;
    pt.pages[id].time = getTime();
    pt.pages[id].visited = 1;
    pt.pages[id].changed = 0;
}

void visit(PageTable &pt, int id, int l, int *ra)
{
    if (!pt.pages[id].state)
    {
        cout << "页面" << id << "不在内存中，从外存中调入" << endl;
        load(pt, id, l, ra);
    }
    pt.pages[id].visited++;
    pt.pages[id].changed = random(0, 2);
}

bool isFull()
{
    if (getChunkID() == -1)
    {
        cout << "内存已满，无法创建进程。" << endl;
        return 1;
    }
    return 0;
}

//创建
void create(int id, int size, PageTable &pt)
{
    pt.length = ceil(size / 4.0);
    pt.progessID = id;
    pt.pages = new Block[pt.length];
    for (int i = 0; i < pt.length; i++)
    {
        pt.pages[i].pageID = i;
        if (random(0, 2))
            load(pt, i, l[id], ra[id]);
    }
}

void print(PageTable pt)
{
    cout << "====================进程名：" << pt.progessID << "====================" << endl;
    cout << "页面号"
         << "\t状态位"
         << "\t页块号"
         << "\t访问位"
         << "\t修改位"
         << "\t载入时刻" << endl;
    for (int i = 0; i < pt.length; i++)
    {
        if (pt.pages[i].state)
            cout << pt.pages[i].pageID << "\t" << pt.pages[i].state << "\t" << pt.pages[i].chunkID << "\t" << pt.pages[i].visited << "\t" << pt.pages[i].changed << "\t" << pt.pages[i].time << endl;
        else
            cout << pt.pages[i].pageID << "\t" << pt.pages[i].state << "\t-\t-\t-\t-" << endl;
    }
    cout << endl;
}

//生成随即访问序列
void randomAccess(int t)
{
    l[t] = random(1, 10);
    for (int i = 0; i < l[t]; i++)
        ra[t][i] = random(0, pagetable[t].length);
}

void *access_fun(void *arg)
{
    long in = (long)arg;
    for (int i = 0; i < l[in]; i++)
    {
        sleep(5);
        pthread_mutex_lock(&rw);
        cout << "进程" << in << "正在访问页面" << ra[in][i] << endl;
        visit(pagetable[in], ra[in][i], l[in], ra[in]);
        print(pagetable[in]);
        pthread_mutex_unlock(&rw);
    }
    return NULL;
}

int main()
{
    srand((unsigned)time(0));
    timeval tv;
    gettimeofday(&tv, NULL);
    start = tv.tv_sec * 1000 * 1000 + tv.tv_usec;
    pthread_t pro[maxm];
    cout << "1.LUR" << endl;
    cout << "2.FIFO" << endl;
    cout << "3.OPT" << endl;
    cout << "请选择置换方式：";
    cin >> r;
    if (r != 1 && r != 2 && r != 3)
        cout << "输入错误！" << endl;
    pthread_mutex_init(&rw, NULL);
    int size, option;
    int t = 0;
    while (t < maxm && !isFull())
    {
        cout << "1.创建进程" << endl;
        cout << "2.结束" << endl;
        cout << "请选择要进行的操作：";
        cin >> option;
        if (option == 2)
            break;
        else if (option == 1)
        {
            cout << "请输入要创建进程的大小：";
            cin >> size;
            create(t, size, pagetable[t]);
            print(pagetable[t]);
            randomAccess(t);
            cout << "进程" << t << "访问序列：";
            for (int i = 0; i < l[t] - 1; i++)
                cout << ra[t][i] << ", ";
            cout << ra[t][l[t] - 1] << endl;
            cout << endl;
            t++;
        }
        else
            cout << "输入错误！" << endl;
    }
    for (long i = 0; i < t; i++)
    {
        int ret = pthread_create(&pro[i], NULL, access_fun, (void *)i);
        if (ret != 0)
        {
            printf("error");
            exit(0);
        }
    }
    for (int i = 0; i < t; i++)
    {
        pthread_join(pro[i], NULL);
    }
}

