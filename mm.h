#ifndef MM_H_INCLUDED
#define MM_H_INCLUDED

#include <omp.h>
#include <iostream>

#ifndef MM_WIDTH
#define MM_WIDTH 512
#endif

#define MM_BLOCK 32
#define MM_MAX_CPU 8
#define MM_HBLOCK MM_BLOCK / 2
#define MM_MATRIX_SIZE MM_WIDTH * MM_WIDTH

using namespace std;

template<typename T>
struct matrix
{
    T *a;
    T *b;
    T *c;
};

template <typename T>
void print_matrix(T *m)
{
    for(int i=0; i<MM_WIDTH; i++)
    {
        for(int j=0; j<MM_WIDTH; j++)
        {
            cout << *(m+i*MM_WIDTH+j) << "  ";
        }
        cout << std::endl;
    }
    cout << std::endl;
}

int stream_size, num_threads;

template <typename T, typename P>
void pmm_ikj(struct matrix<T> *m1, struct matrix<P> *m2)
{
    T *c = (T*)malloc(sizeof(T) * MM_MATRIX_SIZE);
    P *C = (P*)malloc(sizeof(P) * MM_MATRIX_SIZE);

    for(int n=0; n<stream_size; n++)
    {
        #pragma omp parallel shared(c,C) num_threads(num_threads)
        {
            int *idx = (int *) malloc(sizeof(int) * 5);

            #pragma omp for schedule(static)
            for(int i=0; i<MM_WIDTH; i++)
            {
                idx[0] = i*MM_WIDTH;

                for(int j=0; j<MM_WIDTH; j++)
                {
                    *(c+j+i*MM_WIDTH) = 0;
                    *(C+j+i*MM_WIDTH) = 0;
                }

                for(int _k=0; _k<MM_WIDTH; _k+=MM_BLOCK)
                {
                    for(int k=_k; k<_k+MM_HBLOCK; k+=2)
                    {
                        idx[1] = k*MM_WIDTH;
                        idx[2] = (k+MM_HBLOCK) * MM_WIDTH;
                        idx[3] = idx[0]+k;
                        idx[4] = idx[3]+MM_HBLOCK;

                        T a1[] = {
                            *(m1->a+idx[3]),
                            *(m1->a+idx[4])
                        };

                        P a2[] = {
                            *(m2->a+idx[3]),
                            *(m2->a+idx[4])
                        };

                        T *b_temp = m1->b+idx[1], *bb_temp = m1->b+idx[2];
                        P *B_temp = m2->b+idx[1], *BB_temp = m2->b+idx[2];

                        for(int j=0; j<MM_WIDTH; j++)
                        {
                            c[j+idx[0]] += a1[0] * *b_temp++ + a1[1] * *bb_temp++;
                            C[j+idx[0]] += a2[0] * *B_temp++ + a2[1] * *BB_temp++;
                        }
                    }
                }
            }
        }
    }
    m1->c = c;
    m2->c = C;
}

template <typename T, typename P>
void smm_ikj(struct matrix<T> *m1, struct matrix<P> *m2)
{
    int n;

    T *c = (T*)malloc(sizeof(T) * MM_MATRIX_SIZE);
    P *C = (P*)malloc(sizeof(P) * MM_MATRIX_SIZE);

    for(n=0; n<stream_size; n++)
    {
        int i,j,k,_k;
        for(i=0; i<MM_WIDTH; i++)
        {
            for(int j=0; j<MM_WIDTH; j++)
            {
                *(c+j+i*MM_WIDTH) = 0;
                *(C+j+i*MM_WIDTH) = 0;
            }

            for(_k=0; _k<MM_WIDTH; _k+=MM_BLOCK)
            {
                for(k=_k; k<_k+MM_BLOCK; k+=2)
                {
                    int ik = i*MM_WIDTH+k, ik1 = ik+1;

                    T a1_ik = *(m1->a+ik);
                    T a1_ik1 = *(m1->a+ik1);

                    for(j=0; j<MM_WIDTH; j++)
                    {
                        int ij = i*MM_WIDTH+j, kj = k*MM_WIDTH+j, k1j = (k+1)*MM_WIDTH+j;
                        *(c+ij) += a1_ik * *(m1->b+kj) + a1_ik1 * *(m1->b+k1j);
                    }

                    P a2_ik = *(m2->a+ik);
                    P a2_ik1 = *(m2->a+ik1);

                    for(j=0; j<MM_WIDTH; j++)
                    {
                        int ij = i*MM_WIDTH+j, kj = k*MM_WIDTH+j, k1j = (k+1)*MM_WIDTH+j;
                        *(C+ij) += a2_ik * *(m2->b+kj) + a2_ik1 * *(m2->b+k1j);
                    }
                }
            }
        }
    }
    m1->c = c;
    m2->c = C;
}

#endif // MM_H_INCLUDED
