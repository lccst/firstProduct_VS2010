#include "dp_GSL.h"
#include <stdio.h>
#include <malloc.h >
#include <string.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_vector.h>
////#include <math.h>

#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_multifit_nlin.h>

#include "gsl/gsl_errno.h"
#include "TLS.h"
#pragma comment(lib, "libgsl.a")
#pragma comment(lib, "libgslcblas.a")
struct data {
	size_t n;
	double *y;
	double *sigma;
};

int gausse_f (const gsl_vector * x, void *data, gsl_vector * f)
{
	size_t n = ((struct data *)data)->n;
	double *y = ((struct data *)data)->y;
	double *sigma = ((struct data *) data)->sigma;
	double A = gsl_vector_get (x, 0);
	double u = gsl_vector_get (x, 1);
	double c = gsl_vector_get (x, 2);
    double b = gsl_vector_get (x, 3);
	size_t i;
	for (i = 0; i < n; i++){
		// Model Yi = A*exp(-(t-u)*(t-u)/(2*c*c))+b
		double t = sigma[i];
		double Yi = A*exp(-(t-u)*(t-u)/(2*c*c))+b;
		gsl_vector_set (f, i, (Yi - y[i]));
	}
	return GSL_SUCCESS;
}

int gausse_df (const gsl_vector * x, void *data, gsl_matrix * J)
{
	size_t n = ((struct data *)data)->n;
	double *sigma = ((struct data *) data)->sigma;
	double A = gsl_vector_get (x, 0);
	double u = gsl_vector_get (x, 1);
	double c = gsl_vector_get (x, 2);
    double b = gsl_vector_get (x, 3);
	size_t i;
	for (i = 0; i < n; i++){
		/* Jacobian matrix J(i,j) = dfi / dxj, */
		/* where fi = A*exp(-(t-u)*(t-u)/(2*c*c))+b, */
		/* and the xj are the parameters (A,u,c,b) */
		double t = sigma[i];
		gsl_matrix_set (J, i, 0, exp(-(t-u)*(t-u)/(2*c*c)));
		gsl_matrix_set (J, i, 1, A*exp(-(t-u)*(t-u)/(2*c*c))*((t-u)/(c*c)));
		gsl_matrix_set (J, i, 2, A*exp(-(t-u)*(t-u)/(2*c*c))*((t-u)*(t-u)/(c*c*c)));
		gsl_matrix_set (J, i, 3, 1.0);
	}
	return GSL_SUCCESS;
}

int  gausse_fdf (const gsl_vector * x, void *data, gsl_vector * f, gsl_matrix * J)
{
	gausse_f (x, data, f);
	gausse_df (x, data, J);
	return GSL_SUCCESS;
}


void print_state (size_t iter, gsl_multifit_fdfsolver * s)
{
	printf ("iter: %3u x = % 15.8f % 15.8f % 15.8f "
	"|f(x)| = %g\n",
	iter,
	gsl_vector_get (s->x, 0),
	gsl_vector_get (s->x, 1),
	gsl_vector_get (s->x, 2),
	gsl_blas_dnrm2 (s->f));
}

/*****************************************************************
* 与 gsl_vector_view_array 同功能，调用原 gsl_vector_view_array报错
******************************************************************/
WINGSLDLL_API _gsl_vector_view  my_gsl_vector_view_array (double *v, size_t n)
{
	_gsl_vector_view rtn;

	rtn.vector.block = 0;
	rtn.vector.owner = 0;
	rtn.vector.size = n;
	rtn.vector.stride = 1;
	rtn.vector.data = v;

	return rtn;
}

/*****************************************************************
*	fit_x、fit_y：拟合数据的坐标		
*	coef_init: 拟合初值
*	fitLen：拟合数据的长度
*	tolerance：拟合精度
*	返回迭代次数
******************************************************************/
unsigned int gaussse_fit(double*result, double *fit_x, double *fit_y, unsigned int fitLen, double *coef_init, double tolerance)
{
	const gsl_multifit_fdfsolver_type *T;
	gsl_multifit_fdfsolver *s;
	int status;
	unsigned int iter = 0;
	const size_t p = 4;		// 4个参数
	gsl_matrix *covar;
	covar = gsl_matrix_alloc (p, p);

	struct data d = { fitLen, fit_y, fit_x};
	gsl_multifit_function_fdf f;
	gsl_vector_view x ;
	x = my_gsl_vector_view_array(coef_init, p);		
	const gsl_rng_type * type;
	gsl_rng * r;
	gsl_rng_env_setup();
	type = gsl_rng_default;
	r = gsl_rng_alloc (type);
	f.f = &gausse_f;
	f.df = &gausse_df;
	f.fdf = &gausse_fdf;
	f.n = fitLen;
	f.p = p;
	f.params = &d;

	T = gsl_multifit_fdfsolver_lmsder;
	s = gsl_multifit_fdfsolver_alloc (T, fitLen, p);
	gsl_multifit_fdfsolver_set (s, &f, &x.vector);

	//print_state (iter, s);

	do{
		iter++;
		status = gsl_multifit_fdfsolver_iterate(s);
		//printf ("status = %s\n", gsl_strerror (status));
		//print_state (iter, s);
		if (status)
			break;
		status = gsl_multifit_test_delta (s->dx, s->x, tolerance, tolerance);
	}
	while (status == GSL_CONTINUE && iter < 300);
	gsl_multifit_covar (s->J, 0.0, covar);

	#define FIT(i) gsl_vector_get(s->x, i)
	#define ERR(i) sqrt(gsl_matrix_get(covar,i,i))
	{
		for(int i=0; i<p; i++)
			result[i] = FIT(i);
		//double chi = gsl_blas_dnrm2(s->f);
		//double dof = fitLen - p;
		//double c = GSL_MAX_DBL(1, chi / sqrt(dof));
		//printf("chisq/dof = %g\n", pow(chi, 2.0) / dof);
		//printf ("A = %.5f +/- %.5f\n", FIT(0), c*ERR(0));
		//printf ("u = %.5f +/- %.5f\n", FIT(1), c*ERR(1));
		//printf ("c = %.5f +/- %.5f\n", FIT(2), c*ERR(2));
		//printf ("b = %.5f +/- %.5f\n", FIT(3), c*ERR(3));
	}
	//printf ("status = %s\n", gsl_strerror (status));
	gsl_multifit_fdfsolver_free (s);
	gsl_matrix_free (covar);
	gsl_rng_free (r);

	return iter;
}


// 与下位机约定好的，下位机发送的数据是16位的，因此这里做小小的处理
unsigned int getData(HANDLE hCom, void *outbufx, void *outbufy)
{
	unsigned char buf[4] = {0};
	unsigned int  cnt = 0;
	unsigned int  totalCnt = 0;
	unsigned int  data2bytes = 0;

	memset(buf, '\0', sizeof(unsigned char)*1024);

	while(serial_waitData(hCom) == -1);	// 等待数据到来
	while(1){
		cnt = serial_read(hCom, buf, 4);	// 串口发送的数据是单个字节发送的，所以16位的数据需要重新组合一次

		data2bytes = 0;
		data2bytes |= buf[0];
		data2bytes <<= 8;
		data2bytes |= buf[1];
		*((unsigned int*)outbufx+totalCnt) = data2bytes;	// 下位机发送的是坐标，这个是 x 坐标

		data2bytes = 0;
		data2bytes |= buf[2];
		data2bytes <<= 8;
		data2bytes |= buf[3];
		*((unsigned int*)outbufy+totalCnt) = data2bytes;	// 这个是 y 坐标

		totalCnt ++;

		if(data2bytes == 65535)	// 约定发送方，发送 0xffff 作为结束标志
			break;
	}
	
	return totalCnt;
}

/********************************************************************************
*		判断 data，从 startPos 开始，到 startPos+dist 范围内
*		如果存在 data[cnt] 大于 0，则返回 0 ，否则返回 -1
*********************************************************************************/
int isDataUseful_inDistance(double *data, unsigned short size, int startPos, int dist)
{
	int i = 0;
	for(i=startPos; i<startPos+dist; i++){
		if(i>size)
			i = size-1;
		if(data[i] > 0)
			return 0;
	}
	return -1;
}

double aveData(double *data, unsigned short size){
	double rtn = 0.0;
	for(int i=0; i<size; i++)
		rtn += ((double)data[i])/((double)size);
	return rtn;
}

double maxData(double *data, unsigned short size, unsigned short *loc){
	double rtn = 0.0;
	unsigned short loc_tmp = 0;
	for(int i=0; i<size; i++){
		rtn = (rtn>((double)data[i]))?rtn:((double)data[i]);
		loc_tmp= (rtn>((double)data[i]))?(loc_tmp):i;
	}
	*loc = loc_tmp;
	return rtn;
}

int processBuffer(double *data, unsigned short size, int dist, int peakWdthThd){
	int i = 0;
	double *buf;
	double *fit_x, *fit_y;
	double maxValue = 0.0;
	double fit_initValue[4] = {0};
	double fit_result[4] = {0};
printf("--------------------------------------------------------------\n");
	buf = (double *)malloc(sizeof(double)*size);
	fit_x = (double *)malloc(sizeof(double)*size);
	fit_y = (double *)malloc(sizeof(double)*size);

	// 计算阈值，最大值的一半
	unsigned short loc_tmp;
	maxValue = maxData(data, size, &loc_tmp);
	double thd = 0.5*maxValue;
	
	// 大于阈值的保留，小于阈值的归零
	for(i=0; i<size; i++)
		*(buf+i) = (data[i]>thd)?data[i]:0;

	int fs =0, ff=0;		// fs，ff分别是高斯包络的起始点和终点
	int fbg_num = 0;
	for(i=0; i<size; i++){
		if( *(buf+i)>0 ){
			fs = i;			// 第一个不为零的点作为高斯包络的起点
			while(1){
				i++;
				if(i>=size)
					break;
				if( isDataUseful_inDistance(buf,size, i, dist) == -1 ){
					ff = i;		// 参考 isDataUseful_inDistance
					break;
				}
			}
			if((ff-fs)>peakWdthThd){	// 如果高斯包络的点数大于指定阈值，则进一步处理，否则不处理
				fs = (fs-2*dist<0)?0:(fs-2*dist);
				ff = (ff+2*dist>size)?size:(ff+2*dist);	// 多取一点数据用于拟合
				for(i=fs; i<ff; i++){
					*(fit_x+i-fs) = (double)i;
					*(fit_y+i-fs) = (double)(*(buf+i));
				}
				unsigned short locTmp = 0;
				maxValue = maxData(fit_y, ff-fs, &locTmp);
				fit_initValue[0] = maxValue;
				fit_initValue[1] = locTmp+fs;
				fit_initValue[2] = 1.0;
				fit_initValue[3] = aveData(data,size);
				gaussse_fit(fit_result, fit_x, fit_y, ff-fs, fit_initValue);

				fbg_num++;
				printf("fbg%d: %0.3f\n", fbg_num, fit_result[1]);
			}
		}
	}
	free(buf);
	buf = NULL;
	free(fit_x);
	fit_x = NULL;
	free(fit_y);
	fit_y = NULL;

	printf("--------------------------------------------------------------\n\n");
	return 0;
}








//#define N 400
//#define Q 40
//double y[N] = {0};
//
//void main()
//{
//	int i = 0;
//	double tmp[Q];
//	const gsl_rng_type * type;
//	gsl_rng * r;
//	type = gsl_rng_default;
//	r = gsl_rng_alloc (type);
//
//	for(i=0; i<Q; i++){
//		double t = i-Q/2;
//		tmp[i] = 1.5*exp(-(t-0.6)*(t-0.6)/(2*3.4*3.4)) + 0.2
//				+ gsl_ran_gaussian (r, 0.03);
//	}
//
//	for(i=0; i<N; i++)
//		y[i] = gsl_ran_gaussian (r, 0.03);
//
//	for(i=100; i<100+Q; i++)
//		y[i] = tmp[i-100];
//	for(i=300; i<300+Q; i++)
//		y[i] = tmp[i-300];
//
//	//for(i=0; i<N/2; i++)
//	//	printf("%0.4f\n", y[i]);
//	//printf("----------------------------------\n");
//	//for(i=N/2; i<N; i++)
//	//	printf("%0.4f\n", y[i]);
//
//	processBuffer(y,N,10, 5);
//
//	getchar();
//}







//#define N 40
//
//void main()
//{
//	int i = 0;
//	double x[N],y[N];
//	double sigma[N];
//	const gsl_rng_type * type;
//	gsl_rng * r;
//
//	type = gsl_rng_default;
//	r = gsl_rng_alloc (type);
//
//	for(i=0; i<N; i++){
//		double t = i-N/2;
//		x[i] = t;
//		//A*exp(-(t-u)*(t-u)/(2*c*c))+b
//		y[i] = 1.5*exp(-(t-0.6)*(t-0.6)/(2*3.4*3.4)) + 0.2
//		+ gsl_ran_gaussian (r, 0.01);
//		sigma[i] = 0.1;
//		printf ("%g\n", y[i]);
//	}
//
//	double init[4] = {0};
//	init[0] = y[20];
//	init[1] = x[20];
//	init[2] = 1.0;
//	init[3] = 0;
//
//	double coef[4] = {0};
//	unsigned int loopTimes = gaussse_fit(coef, x, y, N, init, 0.00001);
//
//	printf("loopTimes: %d\r\nand the result is:\n", loopTimes);
//	for(i=0; i<4; i++)
//		printf("%0.4f ", coef[i]);
//
//
//	getchar();
//}
















//// Solve Ax = b with LU and cholesky
//int main(int argc, char **argv)
//{
//	printf("=========== tst2 ===========\n");
//	double a_data[] = { 2,1,1,3,2,
//		1,2,2,1,1,
//		1,2,9,1,5,
//		3,1,1,7,1,
//		2,1,5,1,8 };
//
//	double b_data[] = { -2,4,3,-5,1 };
//
//	gsl_vector *x = gsl_vector_alloc (5);
//	gsl_permutation * p = gsl_permutation_alloc (5);
//
//	gsl_matrix_view m 
//		= gsl_matrix_view_array(a_data, 5, 5);
//
//	gsl_vector_view b
//		= gsl_vector_view_array(b_data, 5);
//
//
//	int s;
//
//	gsl_linalg_LU_decomp (&m.matrix, p, &s);
//
//	gsl_linalg_LU_solve (&m.matrix, p, &b.vector, x);
//
//	printf ("x = \n");
//	gsl_vector_fprintf(stdout, x, "%g");
//
//	double a2_data[] = { 2,1,1,3,2,
//		1,2,2,1,1,
//		1,2,9,1,5,
//		3,1,1,7,1,
//		2,1,5,1,8 };
//
//	double b2_data[] = { -2,4,3,-5,1 };
//	gsl_matrix_view m2 = gsl_matrix_view_array(a2_data, 5, 5);
//	gsl_vector_view b2 = gsl_vector_view_array(b2_data, 5);
//	gsl_linalg_cholesky_decomp(&m2.matrix);
//	gsl_linalg_cholesky_solve(&m2.matrix, &b2.vector, x);
//	printf ("x = \n");
//	gsl_vector_fprintf(stdout, x, "%g");
//
//	gsl_permutation_free (p);
//	gsl_vector_free(x);
//	return 0;
//}



//int detect_peak(
//        const double*   data, /* the data */ 
//        int             data_count, /* row count of data */ 
//        int*            emi_peaks, /* emission peaks will be put here */ 
//        int*            num_emi_peaks, /* number of emission peaks found */
//        int             max_emi_peaks, /* maximum number of emission peaks */ 
//        int*            absop_peaks, /* absorption peaks will be put here */ 
//        int*            num_absop_peaks, /* number of absorption peaks found */
//        int             max_absop_peaks, /* maximum number of absorption peaks*/ 
//        double          delta, /* delta used for distinguishing peaks */
//        int             emi_first) /* should we search emission peak first of absorption peak first? */   
//{
//    int     i;
//    double  mx;
//    double  mn;
//    int     mx_pos = 0;
//    int     mn_pos = 0;
//    int     is_detecting_emi = emi_first;
//
//    mx = data[0];
//    mn = data[0];
//    *num_emi_peaks = 0;
//    *num_absop_peaks = 0;
//
//    for(i = 1; i < data_count; ++i){
//        if(data[i] > mx){
//            mx_pos = i;
//            mx = data[i];
//        }
//        if(data[i] < mn){
//            mn_pos = i;
//            mn = data[i];
//        }
//        if(is_detecting_emi && data[i] < mx - delta){
//            if(*num_emi_peaks >= max_emi_peaks) /* not enough spaces */
//                return 1;
//            emi_peaks[*num_emi_peaks] = mx_pos;
//            ++ (*num_emi_peaks);
//            is_detecting_emi = 0;
//            i = mx_pos - 1;
//            mn = data[mx_pos];
//            mn_pos = mx_pos;
//        }
//        else if((!is_detecting_emi) && data[i] > mn + delta){
//            if(*num_absop_peaks >= max_absop_peaks)
//                return 2;
//            absop_peaks[*num_absop_peaks] = mn_pos;
//            ++ (*num_absop_peaks);
//            is_detecting_emi = 1;
//            i = mn_pos - 1;
//            mx = data[mn_pos];
//            mx_pos = data[mn_pos];
//        }
//    }
//    return 0;
//}
