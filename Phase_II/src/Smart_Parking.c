/* SMART PARKING ALLOCATION AND RESERVATION PLATFORM
   Pure C11, single-file version. No external files needed for sample mode.
   Compile: gcc -std=c11 -O2 Smart_Parking.c -o Smart_Parking.exe
   Run Windows PowerShell: .\Smart_Parking.exe
   Optional: Smart_Parking.exe sample_data_folder
   Times are minutes since midnight; reservations use [start,end).
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>
#define MAX_V 512
#define MAX_EDGES 16384
#define MAX_SPACES 512
#define MAX_REQ 4096
#define MAX_ID 64
#define MAX_HEAP 131072
#define INF INT_MAX
typedef enum  {
    REGULAR=0, EV=1
}
VehicleType;
typedef struct  {
    char id[MAX_ID], zone[MAX_ID];
    VehicleType type;
    int node;
}
Space;
typedef struct  {
    char driver[MAX_ID], zone[MAX_ID];
    int start,end;
    VehicleType vehicle;
}
Reservation;
typedef struct  {
    int to,weight,next;
}
Edge;
typedef struct  {
    int V,head[MAX_V],edgeCount;
    Edge edges[MAX_EDGES];
}
Graph;
typedef struct  {
    Reservation request;
    int accepted,spaceIndex,distance,path[MAX_V],pathLen;
    char reason[100];
}
Assignment;
typedef struct  {
    int d,u;
}
HeapItem;
typedef struct  {
    HeapItem data[MAX_HEAP];
    int count;
}
MinHeap;
static Space spaces[MAX_SPACES];
static int spaceCount=0;
static Reservation requests[MAX_REQ];
static int requestCount=0;
static Graph graph;
static Reservation sortedRequests[MAX_REQ],sortTemp[MAX_REQ];
static int bookedIndex[MAX_SPACES][MAX_REQ],bookedCount[MAX_SPACES];
static const char *typeName(VehicleType t) {
    return t==EV?"EV":"REGULAR";
}
static int parseType(const char *s,VehicleType *t) {
    if(!strcmp(s,"REGULAR")) {
        *t=REGULAR;
        return 1;
    }
    if(!strcmp(s,"EV")) {
        *t=EV;
        return 1;
    }
    return 0;
}
static void initGraph(Graph *g,int V) {
    int i;
    g->V=V;
    g->edgeCount=0;
    for(i=0;i<MAX_V;i++)g->head[i]=-1;
}
static int directedEdge(Graph *g,int u,int v,int w) {
    int i;
    if(g->edgeCount>=MAX_EDGES)return 0;
    i=g->edgeCount++;
    g->edges[i].to=v;
    g->edges[i].weight=w;
    g->edges[i].next=g->head[u];
    g->head[u]=i;
    return 1;
}
static int road(Graph *g,int u,int v,int w) {
    if(u<0||v<0||u>=g->V||v>=g->V||w<0||g->edgeCount+2>MAX_EDGES)return 0;
    return directedEdge(g,u,v,w)&&directedEdge(g,v,u,w);
}
static void addSpace(const char *id,const char *zone,VehicleType t,int node) {
    Space *s=&spaces[spaceCount++];
    snprintf(s->id,MAX_ID,"%s",id);
    snprintf(s->zone,MAX_ID,"%s",zone);
    s->type=t;
    s->node=node;
}
static void addRequest(const char *id,int start,int end,VehicleType t,const char *zone) {
    Reservation *r=&requests[requestCount++];
    snprintf(r->driver,MAX_ID,"%s",id);
    snprintf(r->zone,MAX_ID,"%s",zone);
    r->start=start;
    r->end=end;
    r->vehicle=t;
}
static void sampleData(void) {
    initGraph(&graph,7);
    road(&graph,0,1,40);
    road(&graph,0,2,90);
    road(&graph,1,3,20);
    road(&graph,1,4,35);
    road(&graph,2,5,25);
    road(&graph,2,6,40);
    road(&graph,1,2,60);
    road(&graph,4,5,70);
    addSpace("S1","A",REGULAR,3);
    addSpace("S2","A",REGULAR,4);
    addSpace("S3","B",REGULAR,5);
    addSpace("S4","B",EV,6);
    addRequest("D1",540,660,REGULAR,"A");
    addRequest("D2",600,720,REGULAR,"A");
    addRequest("D3",660,780,REGULAR,"A");
    addRequest("D4",570,630,EV,"B");
}
static int skipLine(const char *s) {
    while(isspace((unsigned char)*s))s++;
    return !*s||*s=='#';
}
static int loadData(const char *dir) {
    char path[512],line[512],id[MAX_ID],zone[MAX_ID],type[MAX_ID];
    int V,E,i,u,v,w,start,end,node,n;
    FILE *f;
    VehicleType t;
    snprintf(path,sizeof(path),"%s/graph.txt",dir);
    f=fopen(path,"r");
    if(!f) {
        printf("Load error: cannot open %s\n",path);
        return 0;
    }
    do {
        if(!fgets(line,sizeof(line),f)) {
            fclose(f);
            puts("Load error: missing graph header");
            return 0;
        }
    }
    while(skipLine(line));
    if(sscanf(line,"%d %d",&V,&E)!=2||V<1||V>MAX_V||E<0||E>MAX_EDGES/2) {
        fclose(f);
        puts("Load error: invalid graph header");
        return 0;
    }
    initGraph(&graph,V);
    for(i=0;i<E;i++) {
        if(fscanf(f,"%d %d %d",&u,&v,&w)!=3||!road(&graph,u,v,w)) {
            fclose(f);
            puts("Load error: invalid graph edge");
            return 0;
        }
    }
    fclose(f);
    snprintf(path,sizeof(path),"%s/spaces.txt",dir);
    f=fopen(path,"r");
    if(!f) {
        printf("Load error: cannot open %s\n",path);
        return 0;
    }
    while(fgets(line,sizeof(line),f)) {
        if(skipLine(line))continue;
        if(sscanf(line,"%63s %63s %63s %d",id,zone,type,&node)!=4||!parseType(type,&t)||node<0||node>=V||spaceCount>=MAX_SPACES) {
            fclose(f);
            puts("Load error: invalid space");
            return 0;
        }
        for(n=0;n<spaceCount;n++)if(!strcmp(spaces[n].id,id)) {
            fclose(f);
            puts("Load error: duplicate space ID");
            return 0;
        }
        addSpace(id,zone,t,node);
    }
    fclose(f);
    if(!spaceCount) {
        puts("Load error: no spaces");
        return 0;
    }
    snprintf(path,sizeof(path),"%s/reservations.txt",dir);
    f=fopen(path,"r");
    if(!f) {
        printf("Load error: cannot open %s\n",path);
        return 0;
    }
    while(fgets(line,sizeof(line),f)) {
        if(skipLine(line))continue;
        if(sscanf(line,"%63s %d %d %63s %63s",id,&start,&end,type,zone)!=5||!parseType(type,&t)||start<0||start>=1440||end>1440||end<=start||requestCount>=MAX_REQ) {
            fclose(f);
            puts("Load error: invalid reservation");
            return 0;
        }
        for(n=0;n<requestCount;n++)if(!strcmp(requests[n].driver,id)) {
            fclose(f);
            puts("Load error: duplicate driver ID");
            return 0;
        }
        addRequest(id,start,end,t,zone);
    }
    fclose(f);
    return 1;
}
/* SECTION 1: MERGE SORT - stable chronological ordering */
static void mergeRange(int left,int right) {
    int mid,i,j,k,p;
    if(right-left<=1)return;
    mid=left+(right-left)/2;
    mergeRange(left,mid);
    mergeRange(mid,right);
    i=left;
    j=mid;
    k=left;
    while(i<mid&&j<right) {
        if(sortedRequests[i].start<=sortedRequests[j].start)sortTemp[k++]=sortedRequests[i++];
        else sortTemp[k++]=sortedRequests[j++];
    }
    while(i<mid)sortTemp[k++]=sortedRequests[i++];
    while(j<right)sortTemp[k++]=sortedRequests[j++];
    for(p=left;p<right;p++)sortedRequests[p]=sortTemp[p];
}
static void sortRequests(void) {
    memcpy(sortedRequests,requests,(size_t)requestCount*sizeof(Reservation));
    mergeRange(0,requestCount);
}
static int overlaps(const Reservation *a,const Reservation *b) {
    return a->start<b->end&&b->start<a->end;
}
/* SECTION 2: MIN-HEAP AND DIJKSTRA */
static int less(HeapItem a,HeapItem b) {
    return a.d<b.d||(a.d==b.d&&a.u<b.u);
}
static void heapPush(MinHeap *h,int d,int u) {
    int i,p;
    HeapItem x= {
        d,u
    };
    if(h->count>=MAX_HEAP)return;
    i=h->count++;
    while(i>0) {
        p=(i-1)/2;
        if(!less(x,h->data[p]))break;
        h->data[i]=h->data[p];
        i=p;
    }
    h->data[i]=x;
}
static HeapItem heapPop(MinHeap *h) {
    HeapItem out=h->data[0],x=h->data[--h->count];
    int i=0,c;
    if(h->count==0)return out;
    while((c=2*i+1)<h->count) {
        if(c+1<h->count&&less(h->data[c+1],h->data[c]))c++;
        if(!less(h->data[c],x))break;
        h->data[i]=h->data[c];
        i=c;
    }
    h->data[i]=x;
    return out;
}
static void heapDijkstra(const Graph *g,int source,int dist[],int parent[]) {
    static MinHeap heap;
    int i,e,d,u,v,w,candidate;
    for(i=0;i<g->V;i++) {
        dist[i]=INF;
        if(parent)parent[i]=-1;
    }
    if(source<0||source>=g->V)return;
    heap.count=0;
    dist[source]=0;
    heapPush(&heap,0,source);
    while(heap.count) {
        HeapItem cur=heapPop(&heap);
        d=cur.d;
        u=cur.u;
        if(d!=dist[u])continue;
        for(e=g->head[u];e!=-1;e=g->edges[e].next) {
            v=g->edges[e].to;
            w=g->edges[e].weight;
            if(w<0||d>INF-w)continue;
            candidate=d+w;
            if(candidate<dist[v]) {
                dist[v]=candidate;
                if(parent)parent[v]=u;
                heapPush(&heap,candidate,v);
            }
        }
    }
}
/* SECTION 3: ARRAY-BASED DIJKSTRA - comparison */
static void arrayDijkstra(const Graph *g,int source,int dist[]) {
    int used[MAX_V]= {
        0
    }
    ,step,v,u,e,w;
    for(v=0;v<g->V;v++)dist[v]=INF;
    if(source<0||source>=g->V)return;
    dist[source]=0;
    for(step=0;step<g->V;step++) {
        u=-1;
        for(v=0;v<g->V;v++)if(!used[v]&&(u<0||dist[v]<dist[u]))u=v;
        if(u<0||dist[u]==INF)break;
        used[u]=1;
        for(e=g->head[u];e!=-1;e=g->edges[e].next) {
            v=g->edges[e].to;
            w=g->edges[e].weight;
            if(w>=0&&dist[u]<=INF-w&&dist[u]+w<dist[v])dist[v]=dist[u]+w;
        }
    }
}
/* SECTION 4: GREEDY NEAREST AND FIRST-FIT ALLOCATION */
static int allocate(int firstFit,Assignment out[]) {
    int dist[MAX_V],parent[MAX_V],i,s,j,chosen,best,compatible,occupied,rev[MAX_V],len,node,k;
    sortRequests();
    heapDijkstra(&graph,0,dist,parent);
    memset(bookedCount,0,sizeof(bookedCount));
    for(i=0;i<requestCount;i++) {
        Assignment *a=&out[i];
        const Reservation *r=&sortedRequests[i];
        memset(a,0,sizeof(*a));
        a->request=*r;
        a->spaceIndex=-1;
        a->distance=-1;
        chosen=-1;
        best=INF;
        compatible=0;
        for(s=0;s<spaceCount;s++) {
            const Space *sp=&spaces[s];
            if(strcmp(sp->zone,r->zone)||sp->type!=r->vehicle)continue;
            compatible=1;
            if(sp->node<0||sp->node>=graph.V||dist[sp->node]==INF)continue;
            occupied=0;
            for(j=0;j<bookedCount[s];j++) {
                const Reservation *prior=&sortedRequests[bookedIndex[s][j]];
                if(overlaps(prior,r)) {
                    occupied=1;
                    break;
                }
            }
            if(occupied)continue;
            if(firstFit) {
                chosen=s;
                best=dist[sp->node];
                break;
            }
            if(dist[sp->node]<best||(dist[sp->node]==best&&(chosen<0||strcmp(sp->id,spaces[chosen].id)<0))) {
                chosen=s;
                best=dist[sp->node];
            }
        }
        if(chosen<0) {
            snprintf(a->reason,sizeof(a->reason),"%s",compatible?"No reachable free compatible space":"No space matches zone and vehicle type");
            continue;
        }
        a->accepted=1;
        a->spaceIndex=chosen;
        a->distance=best;
        bookedIndex[chosen][bookedCount[chosen]++]=i;
        len=0;
        node=spaces[chosen].node;
        while(node!=-1&&len<MAX_V) {
            rev[len++]=node;
            node=parent[node];
        }
        a->pathLen=len;
        for(k=0;k<len;k++)a->path[k]=rev[len-1-k];
    }
    return requestCount;
}
static int validate(const Assignment a[],int n,char *error,size_t cap) {
    int i,j,s;
    for(i=0;i<n;i++) {
        if(!a[i].accepted)continue;
        s=a[i].spaceIndex;
        if(s<0||s>=spaceCount||spaces[s].type!=a[i].request.vehicle||strcmp(spaces[s].zone,a[i].request.zone)) {
            snprintf(error,cap,"Invalid space/type/zone assignment");
            return 0;
        }
        for(j=0;j<i;j++)if(a[j].accepted&&a[j].spaceIndex==s&&overlaps(&a[i].request,&a[j].request)) {
            snprintf(error,cap,"Double-booking detected");
            return 0;
        }
    }
    return 1;
}
static void printAssignments(const Assignment a[],int n) {
    int i,j;
    for(i=0;i<n;i++) {
        printf("%s [%d,%d) ",a[i].request.driver,a[i].request.start,a[i].request.end);
        if(!a[i].accepted) {
            printf("REJECTED: %s\n",a[i].reason);
            continue;
        }
        printf("ACCEPTED -> %s | %dm | route ",spaces[a[i].spaceIndex].id,a[i].distance);
        for(j=0;j<a[i].pathLen;j++)printf("%s%d",j?" -> ":"",a[i].path[j]);
        putchar('\n');
    }
}
static volatile unsigned long long benchChecksum=0;
static double elapsedMs(clock_t a,clock_t b) {
    return 1000.0*(double)(b-a)/CLOCKS_PER_SEC;
}
static void report(void) {
    static Assignment nearest[MAX_REQ],first[MAX_REQ];
    int n,i,an=0,af=0,d1[MAX_V],d2[MAX_V],parent[MAX_V],match=1;
    long long dn=0,df=0;
    clock_t t0,t1,t2;
    const int repeats=20000;
    n=allocate(0,nearest);
    allocate(1,first);
    for(i=0;i<n;i++) {
        if(nearest[i].accepted) {
            an++;
            dn+=nearest[i].distance;
        }
        if(first[i].accepted) {
            af++;
            df+=first[i].distance;
        }
    }
    printf("Nearest-space: accepted %d, total distance %lldm\n",an,dn);
    printf("First-fit:     accepted %d, total distance %lldm\n",af,df);
    heapDijkstra(&graph,0,d1,parent);
    arrayDijkstra(&graph,0,d2);
    for(i=0;i<graph.V;i++)if(d1[i]!=d2[i])match=0;
    printf("Heap vs array Dijkstra distances: %s\n",match?"MATCH":"MISMATCH");
    t0=clock();
    for(i=0;i<repeats;i++) {
        heapDijkstra(&graph,0,d1,parent);
        benchChecksum+=(unsigned)(d1[0]);
    }
    t1=clock();
    for(i=0;i<repeats;i++) {
        arrayDijkstra(&graph,0,d2);
        benchChecksum+=(unsigned)(d2[0]);
    }
    t2=clock();
    printf("Heap Dijkstra total %.3f ms for %d runs\n",elapsedMs(t0,t1),repeats);
    printf("Array Dijkstra total %.3f ms for %d runs\n",elapsedMs(t1,t2),repeats);
}
static void discardLine(void) {
    int c;
    while((c=getchar())!='\n'&&c!=EOF) {
    }
}
int main(int argc,char **argv) {
    int choice,i,removed;
    char id[MAX_ID],zone[MAX_ID],type[MAX_ID],err[128];
    int start,end;
    VehicleType vt;
    static Assignment assignments[MAX_REQ];
    if(argc>1) {
        if(!loadData(argv[1]))return 1;
    }
    else sampleData();
    while(1) {
        printf("\nSMART PARKING - PHASE II\n1. View spaces\n2. Add reservation\n3. Show allocations and routes\n4. Cancel reservation by driver ID\n5. Compare algorithms / benchmark\n6. Validate allocations\n0. Exit\nChoice: ");
        if(scanf("%d",&choice)!=1) {
            if(feof(stdin))break;
            discardLine();
            puts("Invalid choice");
            continue;
        }
        if(choice==0)break;
        if(choice==1) {
            for(i=0;i<spaceCount;i++)printf("%s | Zone %s | %s | Node %d\n",spaces[i].id,spaces[i].zone,typeName(spaces[i].type),spaces[i].node);
        }
        else if(choice==2) {
            printf("Enter driverID startMinute endMinute REGULAR/EV zone: ");
            if(scanf("%63s %d %d %63s %63s",id,&start,&end,type,zone)!=5) {
                discardLine();
                puts("Invalid input");
                continue;
            }
            if(start<0||start>=1440||end>1440||end<=start||!parseType(type,&vt)) {
                puts("Invalid time or type");
                continue;
            }
            for(i=0;i<requestCount;i++)if(!strcmp(requests[i].driver,id))break;
            if(i<requestCount) {
                puts("Driver ID already exists");
                continue;
            }
            if(requestCount>=MAX_REQ) {
                puts("Reservation limit reached");
                continue;
            }
            addRequest(id,start,end,vt,zone);
            puts("Request added; choose 3 to recompute allocations");
        }
        else if(choice==3) {
            printAssignments(assignments,allocate(0,assignments));
        }
        else if(choice==4) {
            printf("Driver ID: ");
            if(scanf("%63s",id)!=1) {
                puts("Invalid input");
                continue;
            }
            removed=0;
            for(i=0;i<requestCount;) {
                if(!strcmp(requests[i].driver,id)) {
                    memmove(&requests[i],&requests[i+1],(size_t)(requestCount-i-1)*sizeof(Reservation));
                    requestCount--;
                    removed=1;
                }
                else i++;
            }
            puts(removed?"Reservation removed":"Driver not found");
        }
        else if(choice==5)report();
        else if(choice==6) {
            int n=allocate(0,assignments);
            puts(validate(assignments,n,err,sizeof(err))?"VALIDATION PASSED":err);
        }
        else puts("Unknown option");
    }
    return 0;
}
