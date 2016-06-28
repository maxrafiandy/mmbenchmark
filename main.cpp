#define HACK_TO_REMOVE_DUPLICATE_ERROR

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#ifdef WITH_COUNTER
#include "counter.h"
#endif // WITH_COUNTER
#include "mm.h"

#define DEFAULT_SIZE 128
#define DEFAULT_THREADS 8

using namespace std;

int max_threads;
char *filename, *programmode;
FILE csv_file;

bool print_to_file = false;

template <class foo, class bar>
void run_test(foo m1, bar m2);

struct matrix<float> flp_m1, flp_m2;
struct matrix<int> int_m1, int_m2;

void print_result(double serial, double parallel);
void help(char *argv);

int main(int argc, char * argv[])
{
    int retval = system("clear");
    assert(retval == 0);

    max_threads = omp_get_max_threads();

    int i,opt;
    while ((opt = getopt(argc, argv, "s:t:p")) != -1)
    {
        switch (opt)
        {
        case 's':
            stream_size = atoi(optarg);
            break;
        case 't':
            num_threads = atoi(optarg);
            break;
        case 'p':
            print_to_file = true;
            cout << std::flush;
            break;
        default:
            help(argv[0]);
        }
    }

    if (optind >= argc)
    {
        help(argv[0]);
        exit(EXIT_FAILURE);
    }

    if (print_to_file)
    {
        filename = (char *) malloc(sizeof(char) * sizeof(argv[optind]) + 5);
        strcat(strcpy(filename, argv[optind]),".csv");
    }

    programmode = argv[optind];

    omp_set_dynamic(0);

    srand(time(NULL));

    if (!strcmp("int",programmode))
    {
        int_m1.a = (int*) malloc(MM_MATRIX_SIZE * sizeof(int));
        int_m1.b = (int*) malloc(MM_MATRIX_SIZE * sizeof(int));

        int_m2.a = (int*) malloc(MM_MATRIX_SIZE * sizeof(int));
        int_m2.b = (int*) malloc(MM_MATRIX_SIZE * sizeof(int));
    }

    else if (!strcmp("float",programmode))
    {
        flp_m1.a = (float*) malloc(MM_MATRIX_SIZE * sizeof(float));
        flp_m1.b = (float*) malloc(MM_MATRIX_SIZE * sizeof(float));

        flp_m2.a = (float*) malloc(MM_MATRIX_SIZE * sizeof(float));
        flp_m2.b = (float*) malloc(MM_MATRIX_SIZE * sizeof(float));
    }

    else if (!strcmp("smt",programmode))
    {
        int_m1.a = (int*) malloc(MM_MATRIX_SIZE * sizeof(int));
        int_m1.b = (int*) malloc(MM_MATRIX_SIZE * sizeof(int));

        flp_m2.a = (float*) malloc(MM_MATRIX_SIZE * sizeof(float));
        flp_m2.b = (float*) malloc(MM_MATRIX_SIZE * sizeof(float));
    }

    else
    {
        cout << programmode << " is not a valid mode." << endl;
        help(argv[0]);
    }

    /* Init default values of matrix */
    #pragma omp parallel for private(i) shared(int_m1, int_m2, flp_m1, flp_m2)
    for(i=0; i<MM_WIDTH; i++)
    {
        int j;
        int ix = i*MM_WIDTH;
        for(j=0; j<MM_WIDTH; j++)
        {
            if (!strcmp("int",programmode))
            {

                int ixj = ix+j;
                int_m1.a[ixj] = rand() % 10 + 1;
                int_m1.b[ixj] = rand() % 10 + 1;

                int_m2.a[ixj] = rand() % 10 + 1;
                int_m2.b[ixj] = rand() % 10 + 1;
            }

            else if (!strcmp("float",programmode))
            {
                int ixj = ix+j;
                flp_m1.a[ixj] = (rand() % 10 + 1)/3.14;
                flp_m1.b[ixj] = (rand() % 10 + 1)/3.14;

                flp_m2.a[ixj] = (rand() % 10 + 1)/3.14;
                flp_m2.b[ixj] = (rand() % 10 + 1)/3.14;
            }

            else if (!strcmp("smt",programmode))
            {
                int ixj = ix+j;
                int_m1.a[ixj] = rand() % 10 + 1;
                int_m1.b[ixj] = rand() % 10 + 1;

                flp_m2.a[ixj] = (rand() % 10 + 1)/3.14;
                flp_m2.b[ixj] = (rand() % 10 + 1)/3.14;
            }
        }
    }
#ifdef WITH_COUNTER
#ifdef PCM_FORCE_SILENT
    null_stream nullStream1, nullStream2;
    std::cout.rdbuf(&nullStream1);
    std::cerr.rdbuf(&nullStream2);
#endif


    string program = string(argv[0]);
    EventSelectRegister regs[NUMBER_EVENTS];
    PCM::ExtendedCustomCoreEventDescription conf;
    conf.fixedCfg = NULL; // default
    conf.nGPCounters = NUMBER_EVENTS;
    conf.gpCounterCfg = regs;

    PCM * m = PCM::getInstance();

    m->resetPMU();

    try
    {
        build_event(L1_HITS,&regs[0],0);
        build_event(L2_HITS,&regs[1],1);
        build_event(L3_HITS,&regs[2],2);
        build_event(L3_MISSES,&regs[3],3);
    }
    catch (const char * /* str */)
    {
        exit(EXIT_FAILURE);
    }

    conf.OffcoreResponseMsrValue[0] = events[0].msr_value;
    conf.OffcoreResponseMsrValue[1] = events[1].msr_value;

    PCM::ErrorCode status = m->program(PCM::EXT_CUSTOM_CORE_EVENTS, &conf);
    switch (status)
    {
    case PCM::Success:
        break;
    case PCM::MSRAccessDenied:
        cerr << "Access to Intel(r) Performance Counter Monitor has denied (no MSR or PCI CFG space access)." << endl;
        exit(EXIT_FAILURE);
    case PCM::PMUBusy:
        m->resetPMU();
        cerr << "PMU configuration has been reset. Try to rerun the program again." << endl;
        exit(EXIT_FAILURE);
    default:
        cerr << "Access to Intel(r) Performance Counter Monitor has denied (Unknown error)." << endl;
        exit(EXIT_FAILURE);
    }

    cerr << "\nDetected "<< m->getCPUBrandString() << " \"Intel(r) microarchitecture codename "<<m->getUArchCodename()<<"\""<<endl;

    uint64 BeforeTime = 0, AfterTime = 0;
    SystemCounterState SysBeforeState, SysAfterState;
    const uint32 ncores = m->getNumCores();
    std::vector<CoreCounterState> BeforeState, AfterState;
    std::vector<SocketCounterState> DummySocketStates;

    BeforeTime = m->getTickCount();
    m->getAllCounterStates(SysBeforeState, DummySocketStates, BeforeState);
#endif // WITH_COUNTER

    if (!strcmp("int",programmode)) run_test(int_m1, int_m2);
    else if (!strcmp("float",programmode)) run_test(flp_m1, flp_m2);
    else if (!strcmp("smt",programmode)) run_test(int_m1, flp_m2);

#ifdef WITH_COUNTER
    AfterTime = m->getTickCount();
    m->getAllCounterStates(SysAfterState, DummySocketStates, AfterState);
    FILE *file;
    if (print_to_file)
    {
        file = fopen(filename,"a+");
    }
    cout << "Time elapsed: "<<dec<<fixed<<AfterTime-BeforeTime<<" ms\n";

    cout << "Core | IPC | L1 HIT | L1 MISS | L2 HIT | L2 MISS | L3 HIT | L3 MISS\n";
    double ipc = 0;
    for(uint32 i = 0; i<ncores ; ++i)
    {
        if(m->isCoreOnline(i) == false) continue;
        cout <<" "<< setw(3) << i << "  " << setw(2) << setprecision(2);
        core_events[i] = print_custom_stats(BeforeState[i], AfterState[i], false);
        ipc += core_events[i].ipc;
    }
    cout << "-------------------------------------------------------------------------------------------------------------------\n";
    cout << "   *   ";
    system_event = print_custom_stats(SysBeforeState, SysAfterState, false);
    if (print_to_file)
    {
        fprintf(file,"%lld; %lld; %lld; %lld; %lld; %lld; %.2f;\n", \
                (long long)system_event.l1_hits, (long long)system_event.l1_misses,\
                (long long)system_event.l2_hits, (long long)system_event.l2_misses,\
                (long long)system_event.l3_hits, (long long)system_event.l3_misses, ipc);
        fclose(file);
    }
    swap(BeforeTime, AfterTime);
    swap(BeforeState, AfterState);
    swap(SysBeforeState, SysAfterState);
#endif // WITH_COUNTER

    return EXIT_SUCCESS;
}

template <class foo, class bar>
void run_test(foo m1, bar m2)
{
    double start=0, serial=0, parallel=0;

#ifndef PARALLEL_ONLY
    start = omp_get_wtime();
    smm_ikj(&m1, &m2);
    serial = omp_get_wtime()-start;
#endif // PARALLEL_ONLY

#ifndef SERIAL_ONLY
    start = omp_get_wtime();
    pmm_ikj(&m1, &m2);
    parallel = omp_get_wtime()-start;
#endif // SERIAL_ONLY

    print_result(serial,parallel);
}

void print_result(double serial, double parallel)
{
    cout << std::fixed;
    cout << "SUMMARY   : " << endl;
    cout << "Run Mode     : " << programmode << endl;
    cout << "Stream Size  : " << stream_size << endl;
#ifndef SERIAL_ONLY
    cout << "Threads      : " << num_threads << endl;
#endif // SERIAL_ONLY
#ifndef PARALLEL_ONLY
    cout << "Serial       : " << serial << " second(s)" << endl;
#endif // PARALLEL_ONLY
#ifndef SERIAL_ONLY
    cout << "Parallel     : " << parallel << " second(s)" << endl;
#endif // SERIAL_ONLY
#if !defined PARALLEL_ONLY && !defined SERIAL_ONLY
    cout << "Speedup      : " << serial/parallel << endl;
#endif // PARALLEL_ONLY
    if (print_to_file)
    {
        FILE *file = fopen(filename,"a+");
#ifdef SERIAL_ONLY
        fprintf(file,"%d; %d; %0.5f;\n", 1, stream_size, serial);
#endif // SERIAL_ONLY
#ifdef PARALLEL_ONLY
        fprintf(file,"%d; %d; %0.5f;", num_threads, stream_size, parallel);
#endif // PARALLEL_ONLY
#if !defined PARALLEL_ONLY && !defined SERIAL_ONLY
        fprintf(file,"%d; %d; %0.5f; %0.5f; %0.5f;\n", num_threads, stream_size, serial, parallel, serial/parallel);
#endif
        fclose(file);
    }
}

void help(char *argv)
{
    printf("Usage: %s [OPTION] [MODE]\n", argv);
    printf("OPTION           :\n");
    printf("  -s stream-size : Set number of streaming loop (default %d)\n", DEFAULT_SIZE);
    printf("  -t thread-size : Set number of thread (default %d)\n", DEFAULT_THREADS);
    printf("  -p             : write results into csv file\n");
    printf("\nMODE             :\n");
    printf("  smt            : Run benchmark on integer-float intensive mode\n");
    printf("  int            : Run benchmark on integer intensive mode\n");
    printf("  float          : Run benchmark on float intensive mode\n");
    exit(EXIT_FAILURE);
}
