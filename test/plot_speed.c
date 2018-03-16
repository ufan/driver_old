#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>

#define PORT 8899

int plottype;
char* setupfile;
char* format;

char* title[]={"w0", "r0", "w1", "r1"};

struct speed {
    struct speed *prev, *next;
    int size, num;
    float f[1];
};

struct speed* speed;

static int
xrecv(int s, int n, int* v)
{
    int res, rest=n*4;
    char* p=(char*)v;
    while (rest) {
        res=recv(s, p, rest, 0);
        if (res<0) {
            if (errno!=EINTR) {
                perror("recv");
                return -1;
            } else
                res=0;
        } else if (res==0) {
            fprintf(stderr, "no more data\n");
            return -1;
        }
        rest-=res;
        p+=res;
    }
    return 0;
}

static int
add_entry(struct speed* e)
{
    struct speed *prev, *next;
/* original e->f[i] is measured in Bytes/s */
/* size is in terms of Words */
    switch (plottype) {
    case 0:
        break;
    case 1: {
            int i;
            for (i=e->num-1; i>=0; i--) {
                e->f[i]=4./e->f[i];
            }
        }
        break;
    case 2: {
            int i;
            for (i=e->num-1; i>=0; i--) {
                e->f[i]=(e->size*4.)/e->f[i];
            }
        }
        break;
    default:
        fprintf(stderr, "Program error; plottype %d not implemented\n",
            plottype);
    }

    next=speed; prev=0;

    while (next && (next->size<=e->size)) {prev=next; next=next->next;}
    fprintf(stderr, "size=%d; ", e->size);
    if (prev)
        fprintf(stderr, "prev.size=%d; ", prev->size);
    else
        fprintf(stderr, "prev=0; ");
    if (next)
        fprintf(stderr, "next.size=%d\n", next->size);
    else
        fprintf(stderr, "next=0\n");

    if (next) {
        e->next=next;
        next->prev=e;
    } else
        e->next=0;
    if (prev) {
        e->prev=prev;
        prev->next=e;
    } else {
        e->prev=0;
        speed=e;
    }

    return 0;
}

static void
dump_speed(void)
{
    struct speed *e=speed;
    fprintf(stderr, "---------------\n");
    while (e) {
        int i;
        fprintf(stderr, "%6d", e->size);
        for (i=0; i<e->num; i++)
                fprintf(stderr, " %12.2f\n", e->f[i]);
        fprintf(stderr, "\n");
        e=e->next;
    }
}

static int
setup_plot(char* name)
{
    struct stat buf;
    static time_t last_mtime=0;

    if (stat(name, &buf)<0) {
        fprintf(stderr, "cannot stat \"%s\": %s\n", name, strerror(errno));
        return -1;
    }
    
    if (buf.st_mtime!=last_mtime) {
        FILE* f;
        char s[1024];

        f=fopen(name, "r");
        if (!f) {
            fprintf(stderr, "cannot open \"%s\": %s\n", name, strerror(errno));
            return -1;
        }
        while (fgets(s, 1024, f)) {
            printf("%s\n", s);
        }
        fclose(f);
        last_mtime=buf.st_mtime;
    }
    return 0;
}

static void
plot_it(void)
{
    struct speed *e;
    int i;

    if (!speed) return;

    setup_plot(setupfile);

    printf("plot ");
    for (i=0; i<speed->num; i++)
        printf("\"-\" us 1:2 title '%s' w l%s",
                title[i], (i<speed->num-1)?", ":"\n");

    for (i=0; i<speed->num; i++) {
        e=speed;
        while (e) {
            printf(format, e->size, e->f[i]);
            e=e->next;
        }
        printf("e\n\n");
    }
    fflush(stderr);
}

static void
init_plot(void)
{
    printf("set term x11\n");
    switch (plottype) {
    case 0:
        printf("set title \"Throughput\"\n");
        printf("set xlabel \"Size/words\"\n");
        printf("set ylabel \"Byte/s\"\n");
        printf("plot \"-\" with points\n");
        printf("0 0\n1 1\ne\n\n");
        break;
    case 1:
        printf("set title \"Seconds/Word\"\n");
        printf("set xlabel \"Size/words\"\n");
        printf("set ylabel \"s/word\"\n");
        printf("plot \"-\" with points\n");
        printf("0 0\n1 0\ne\n\n");
        break;
    case 2:
        printf("set title \"Duration\"\n");
        printf("set xlabel \"Size/words\"\n");
        printf("set ylabel \"s\"\n");
        printf("plot \"-\" with points\n");
        printf("0 0\n1 1\ne\n\n");
        break;
    default:
        fprintf(stderr, "Program error; plottype %d not implemented\n",
            plottype);
    }
}

static void
printusage(int argc, char* argv[])
{
    fprintf(stderr, "usage: %s [-s setupfile] [-t type] | gnuplot\n", argv[0]);
    fprintf(stderr, "       -t: 0: bytes/s (default)\n"
                    "           1: s/word\n"
                    "           2: s (absolute)\n");
    fprintf(stderr, "       -s: setupfile contains arbitrary gnuplot commands\n"
                    "           it is reread before plotting each ntuple\n"
                    "           default is 'gnuplot.ini'\n");
}

static int
getoptions(int argc, char* argv[])
{
    extern char *optarg;
    extern int optind;
    int errflag, c;
    const char* args="t:s:";

    setupfile="gnuplot.ini";
    plottype=0;

    optarg=0; errflag=0;
    
    while (!errflag && ((c=getopt(argc, argv, args))!=-1)) {
        switch (c) {
        case 't': plottype=atoi(optarg); break;
        case 's': setupfile=optarg; break;
        default: errflag=1;
        }
    }

    if (errflag || optind!=argc) {
        printusage(argc, argv);
        return -1;
    }

    if ((plottype<0) || (plottype>2)) {
        fprintf(stderr, "plottype %d not known\n", plottype);
        return -1;
    }

    return 0;
}

int main(int argc, char* argv[])
{
    int s, ns;
    struct sockaddr_in addr;
    struct sockaddr caddr;
    struct in_addr in_addr;
    int tmp, res;

    if (getoptions(argc, argv)<0) return 1;
    switch (plottype) {
    case 0:
        format="%6d %12.2f\n";
        break;
    case 1:
        format="%6d %g\n";
        break;
    case 2:
        format="%6d %g\n";
        break;
    default:
        fprintf(stderr, "Program error; plottype %d not implemented\n",
            plottype);
    }

    res=setvbuf(stdout, 0, _IONBF, 0);
    if (res<0) {perror("setvbuf"); return 1;}

    speed=0;

    init_plot();

    bzero(&addr, sizeof(struct sockaddr_in));
    bzero(&caddr, sizeof(struct sockaddr));
    addr.sin_family=AF_INET;
    addr.sin_port=htons(PORT);
    addr.sin_addr.s_addr=INADDR_ANY;
    s=socket(addr.sin_family, SOCK_STREAM, 0);
    if (s<0)
        {perror("socket"); return 1;}
    if (bind(s, (struct sockaddr*)&addr, sizeof(struct sockaddr_in))<0)
        {perror("bind"); return 1;}
    if (listen(s, 1)<0) {perror("listen"); return 1;}
    tmp=sizeof(struct sockaddr_in);
    ns=accept(s, (struct sockaddr*)&addr, &tmp);
    if (ns<0) {perror("accept"); return 1;}
    in_addr.s_addr=addr.sin_addr.s_addr;
    fprintf(stderr, "%s accepted\n", inet_ntoa(in_addr));

    do {
        int n, i;
        struct speed *e;

        if (xrecv(ns, 1, &n)<0) return 1;

        e=malloc(sizeof(struct speed)+(n-1)*sizeof(float));
        if (!e) {
            perror("malloc");
            return -1;
        }
        e->num=n;

        if (xrecv(ns, 1, &e->size)<0) return 1;
        for (i=0; i<n; i++) {
            if (xrecv(ns, 1, (int*)(e->f+i))<0) return 1;
        }
        add_entry(e);
        plot_it();
    } while (1);

    return 0;
}
