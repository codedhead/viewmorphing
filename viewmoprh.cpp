#include "viewmorph.h"
#include "morph_gpu.h"


#include <eigen/core>
#include <eigen/svd>
#include <eigen/eigen>

#include <QLineF>
#include <QPointF>
#include <QVector2D>
#include <QVector>
#include <QImage>

#include <iostream>
#include <fstream>
using namespace std;

#include <assert.h>

using namespace Eigen;

//#define USE_BSGP

//#define DBG_PRINT

#ifdef DBG_PRINT
#define DBG_COUT(xx) cout<<xx
#else
#define DBG_COUT(xx)
#endif


namespace viewmorph
{
	typedef double ele_type;

	typedef Matrix<ele_type,3,Dynamic> mat_3x;
	typedef Matrix<ele_type,3,3> mat_33;
	typedef Matrix<ele_type,4,2> mat_42;
	typedef Matrix<ele_type,8,8> mat_88;
	typedef Matrix<ele_type,Dynamic,9> mat_x9;
	typedef Matrix<ele_type,Dynamic,Dynamic> mat;
	typedef Matrix<ele_type,Dynamic,1> colvec;
	typedef Matrix<ele_type,3,1> colvec3;
	typedef Matrix<ele_type,4,1> colvec4;
	typedef Matrix<ele_type,8,1> colvec8;
	typedef Matrix<ele_type,1,Dynamic> rowvec;
	typedef colvec vec;

	// collect corresponding points from feature lines, discard those points that are close(actually the sample point)
	void collect_points(const QLineF* flines1,const QLineF* flines2,int lines_num,mat_3x& x1,mat_3x& x2)
	{
		QVector<int> pis;
		pis.reserve(lines_num*2);

#define MIN_DIST (4*4)
		
		int j,jLen;
		for(int i=0;i<lines_num;++i)
		{
			for(j=0,jLen=pis.size();j<jLen;++j)
			{
				if(j%2==0)
				{
					if(QVector2D(flines1[i].p1()-flines1[pis[j]/2].p1()).lengthSquared()<MIN_DIST)
						break; // consider as the same point
				}
				else 
				{
					if(QVector2D(flines1[i].p1()-flines1[pis[j]/2].p2()).lengthSquared()<MIN_DIST)
						break; // consider as the same point
				}
			}
			if(j>=jLen) // past the test
			{
				pis.push_back(i*2);
			}

			for(j=0,jLen=pis.size();j<jLen;++j)
			{
				if(pis[j]%2==0)
				{
					if(QVector2D(flines1[i].p2()-flines1[pis[j]/2].p1()).lengthSquared()<MIN_DIST)
						break; // consider as the same point
				}
				else 
				{
					if(QVector2D(flines1[i].p2()-flines1[pis[j]/2].p2()).lengthSquared()<MIN_DIST)
						break; // consider as the same point
				}
			}
			if(j>=jLen) // past the test
			{
				pis.push_back(i*2+1);
			}
		}

		x1.resize(3,pis.size());
		x2.resize(3,pis.size());
		for(j=0,jLen=pis.size();j<jLen;++j)
		{
			if(pis[j]%2==0)
			{
				x1(0,j)=flines1[pis[j]/2].x1();
				x1(1,j)=flines1[pis[j]/2].y1();
				x1(2,j)=1.f;
				
				x2(0,j)=flines2[pis[j]/2].x1();
				x2(1,j)=flines2[pis[j]/2].y1();
				x2(2,j)=1.f;
			}
			else
			{
				x1(0,j)=flines1[pis[j]/2].x2();
				x1(1,j)=flines1[pis[j]/2].y2();
				x1(2,j)=1.f;

				x2(0,j)=flines2[pis[j]/2].x2();
				x2(1,j)=flines2[pis[j]/2].y2();
				x2(2,j)=1.f;
			}
		}
	}

	void normalize_coords(mat_3x& x,mat_33& T)
	{
		mat_3x save_x=x;
		int pts_num=x.cols();

		DBG_COUT("pts: \n"<<x<<endl);

		ele_type mean_x=x.row(0).sum()/pts_num,
			mean_y=x.row(1).sum()/pts_num;

		DBG_COUT("centroid: "<<mean_x<<", "<<mean_y<<endl);
		// translate
		x.row(0).array() -= mean_x;
		x.row(1).array() -= mean_y;

		rowvec dist=(x.row(0).array().square().array()+x.row(1).array().square().array()).array().sqrt();
		DBG_COUT("dist: \n"<<dist<<endl);

		ele_type meandist=dist.sum()/pts_num;
		DBG_COUT("meandist: "<<meandist<<endl);

		// force mean dist sqrt(2)
		float scale=sqrt(2.f)/meandist;
		T	<< scale, 0.f, -scale*mean_x,
			0.f, scale, -scale*mean_y,
			0.f, 0.f, 1.f;

		DBG_COUT("T: \n"<<T<<endl);
		x=T*save_x;
		DBG_COUT("normalized: \n"<<x<<endl);
	}



	mat_33 compute_F(const QLineF* flines1,const QLineF* flines2,int lines_num)
	{
		
		mat_33 T1,T2;
		mat_3x x1,x2;

		collect_points(flines1,flines2,lines_num,x1,x2);

		std::ofstream file1("x1.txt");
		file1<<x1;
		std::ofstream file2("x2.txt");
		file2<<x2;
		//x1.save("x1.mat",raw_ascii);
		//x2.save("x2.mat",raw_ascii);

		normalize_coords(x1,T1);
		normalize_coords(x2,T2);
		

		int pts_num=x1.cols();
		mat_x9 A;
		A.resize(pts_num,9);
		mat U,V;
		vec D;
		
		colvec x1_row1_t=x1.row(0).transpose(),
			x1_row2_t=x1.row(1).transpose(),
			x2_row1_t=x2.row(0).transpose(),
			x2_row2_t=x2.row(1).transpose();

		// matlab .*
		A.col(0)=x2_row1_t.cwiseProduct(x1_row1_t);
		A.col(1)=x2_row1_t.cwiseProduct(x1_row2_t);
		A.col(2)=x2_row1_t;
		A.col(3)=x2_row2_t.cwiseProduct(x1_row1_t);
		A.col(4)=x2_row2_t.cwiseProduct(x1_row2_t);
		A.col(5)=x2_row2_t;
		A.col(6)=x1_row1_t;
		A.col(7)=x1_row2_t;
		A.col(8).fill(1);//ones(pts_num,1);

		JacobiSVD<mat_x9> svdsolver;
		svdsolver.compute(A,ComputeFullU|ComputeFullV|ComputeEigenvectors);
		U=svdsolver.matrixU();
		V=svdsolver.matrixV();
		D=svdsolver.singularValues();


		DBG_COUT("V(:,8)\n"<<V.col(8)<<endl);
		DBG_COUT("D: \n"<<D<<endl);


		mat_33 F;
		// the least eigen vector (last column)
		colvec least_eigv=V.col(8);
		// F=reshape(V.col(8),3,3).transpose();
		F<<least_eigv(0),least_eigv(1),least_eigv(2),
			least_eigv(3),least_eigv(4),least_eigv(5),
			least_eigv(6),least_eigv(7),least_eigv(8);

		
		

		JacobiSVD<mat_33> svd33solver;
		svd33solver.compute(F,ComputeFullU|ComputeFullV|ComputeEigenvectors);
		U=svd33solver.matrixU();
		V=svd33solver.matrixV();
		D=svd33solver.singularValues();

		DBG_COUT("D: \n"<<D<<endl);
		D(2)=0.f;
		// force rank 2
		F=U*D.asDiagonal()*V.transpose();

		DBG_COUT("Fnorm: "<<F<<endl);

		// transform back to, corresponds to original coords
		F=T2.transpose()*F*T1;
		DBG_COUT("F: "<<F<<endl);

		return F;
	}

	mat_33 Rmatrix(vec axis,float angle) // z==0.f
	{
		float x=axis(0),y=axis(1);
		float c=cosf(angle),s=sinf(angle),t=1-c;
		
		mat_33 R;
		R<<t*x*x+c,t*x*y,s*y,
		 t*x*y,t*y*y+c,-s*x,
		 -s*y,s*x,c;
		return R;
	}
	// rotate around z
	mat_33 RZmatrix(float angle)
	{
		mat_33 R;
		R<<cosf(angle),-sinf(angle),0.f,
			sinf(angle),cosf(angle),0.f,
			0.f,0.f,1.f;
		return R;
	}

	void compute_H0H1(const mat_33& F,mat_33& invH0,mat_33& invH1)
	{
		EigenSolver<mat_33> eig33solver;
		VectorXcd eig_vals;
		MatrixXcd eig_vecs;

		int e0i,e1i;
		colvec3 e0,e1,d0,d1;

		eig33solver.compute(F);
		eig_vals=eig33solver.eigenvalues();
		eig_vecs=eig33solver.eigenvectors();
		DBG_COUT("F eig vals: \n"<<eig_vals.real()<<endl);
		DBG_COUT("F eig vecs: \n"<<eig_vecs.real()<<endl);
		eig_vals.real().cwiseAbs().minCoeff(&e0i); // treat the min(abs) eigen value as 0 eigen value
		e0=eig_vecs.col(e0i).real();

		eig33solver.compute(F.transpose());
		eig_vals=eig33solver.eigenvalues();
		eig_vecs=eig33solver.eigenvectors();
		DBG_COUT("F' eig vals: \n"<<eig_vals.real()<<endl);
		DBG_COUT("F' eig vecs: \n"<<eig_vecs.real()<<endl);
		eig_vals.real().cwiseAbs().minCoeff(&e1i);
		e1=eig_vecs.col(e1i).real();

		DBG_COUT("e0: "<<e0<<endl);
		DBG_COUT("e1: "<<e1<<endl);


		// get d0,d1
		d0<<-e0(1),e0(0),0.f;
		DBG_COUT("d0: \n"<<d0<<endl);
		d0/=d0.norm();
		DBG_COUT("d0 normed: \n"<<d0<<endl);
		
		colvec Fd0=F*d0;
		d1<<-Fd0(1),Fd0(0),0.f;
		DBG_COUT("d1: \n"<<d1<<endl);
		d1/=d1.norm();
		DBG_COUT("d1 normed: \n"<<d1<<endl);

		// get theta0, theta1;
		float theta0,theta1;
		theta0=atan(e0(2)/(d0(1)*e0(0)-d0(0)*e0(1)));
		theta1=atan(e1(2)/(d1(1)*e1(0)-d1(0)*e1(1)));
		printf("theta0: %f\ntheta1: %f\n",theta0,theta1);

		// the R0, R1 rotation matrix ( theta around d )
		mat_33 R0=Rmatrix(d0,theta0),
			R1=Rmatrix(d1,theta1),
			R0_inv=Rmatrix(d0,-theta0);

		DBG_COUT("R0: \n"<<R0<<endl);
		DBG_COUT("R1: \n"<<R1<<endl);

		// get phi0, phi1
		colvec new_e0,new_e1;
		new_e0=R0*e0;
		new_e1=R1*e1;

		DBG_COUT("new_e0: \n"<<new_e0<<endl);
		DBG_COUT("new_e1: \n"<<new_e1<<endl);

		float phi0,phi1;
		phi0=-atanf(new_e0(1)/new_e0(0));
		phi1=-atanf(new_e1(1)/new_e1(0));
		printf("phi0: %f\nphi1: %f\n",phi0,phi1);

		// the Rphi rotation matrix ( align scan-line, rotate around z-axis )
		mat_33 Rphi0=RZmatrix(phi0),
			Rphi1=RZmatrix(phi1),
			Rphi0_inv=RZmatrix(-phi0);

		// get the T ( correcting the F )
		mat_33 Fbar=Rphi1*R1*F*R0_inv*Rphi0_inv;
		// Fbar to the form:
		//
		//	0 0 0
		//	0 0 a
		//	0 1 b
		//
		Fbar/=Fbar(2,1);
		DBG_COUT("Fbar: \n"<<Fbar<<endl);
		float a=Fbar(1,2),
			b=Fbar(2,2);
		//assert(a*b<=0.f);
		if(a*b>0.f)
		{
			printf("Epipole lines up-down direction error, may be caused the bad fundamental matrix estimation.\n");
		}
// 		if(a*b>0.f)
// 		{
// 			phi1+=M_PI;
// 			Rphi1=RZmatrix(phi1);
// 			// do it again
// 
// 			Fbar=Rphi1*R1*F*R0_inv*Rphi0_inv;
// 			Fbar/=Fbar(2,1);
// 			DBG_COUT("Fbar(+pi): \n"<<Fbar<<endl);
// 			a=Fbar(1,2),
// 			b=Fbar(2,2);
// 		}
		// T has the form:
		//
		//	1 0 0
		//	0 -a -b
		//	0 0 1
		//
		// so that inv(T)'*Fbar=F_parallel
		mat_33 T;
		T<<1,0,0,
		 0,-a,-b,
		 0,0,1;

		//(pinv(T).transpose()*Fbar).print("inv(T)'*Fbar:");

		// NOTE!!! we are calculating the inverses of H0 and H1
		invH0=Rphi0*R0;
		invH1=T*Rphi1*R1;

		DBG_COUT("invH0: \n"<<invH0<<endl);
		DBG_COUT("invH1: \n"<<invH1<<endl);

		cout<<"H1'*F*H0: \n"<<invH1.inverse().transpose()*F*invH0.inverse()<<endl;
	}

	QRgb bg_color=qRgb(198,208,224);

	// directly warp the original image
	void imtransform(QImage& Iwarped,mat_33& M) // mat is the matrix, not inverse
	{
		QImage Isrc=Iwarped.copy();
		Iwarped.fill(0);
		int w=Isrc.width(),h=Isrc.height();

		colvec pt(3);
		int wx,wy;
		for(int y=0;y<h;++y)
			for(int x=0;x<w;++x)
			{
				pt(0)=x;
				pt(1)=y;
				pt(2)=1.f;

				pt=M*pt;

				wx=(int)(pt(0)/pt(2));
				wy=(int)(pt(1)/pt(2));

				if(wx>=0&&wx<w&&wy>=0&&wy<h)
					Iwarped.setPixel(wx,wy,Isrc.pixel(x,y));
			}
	}
	void backward_imtransform(QImage& Iwarped,mat_33& M) // mat is the matrix, not inverse
	{
		QImage Isrc=Iwarped.copy();
		Iwarped.fill(0);
		int w=Isrc.width(),h=Isrc.height();

		colvec pt(3);
		int sx,sy;
		// for every pixel in dst image
		for(int y=0;y<h;++y)
			for(int x=0;x<w;++x)
			{
				pt(0)=x;
				pt(1)=y;
				pt(2)=1.f;

				pt=M*pt;

				sx=(int)(pt(0)/pt(2));
				sy=(int)(pt(1)/pt(2));

				if(sx>=0&&sx<w&&sy>=0&&sy<h)
					Iwarped.setPixel(x,y,Isrc.pixel(sx,sy));
				else
					Iwarped.setPixel(x,y,bg_color);
			}
	}

	// forward transform
	void imtransform(QLineF* flines,int lines_num,mat_33& M)
	{
		colvec pt(3);
		for(int i=0;i<lines_num;++i)
		{
			pt(0)=flines[i].x1();pt(1)=flines[i].y1();pt(2)=1.f;
			pt=M*pt;
			flines[i].setP1(QPointF(pt(0)/pt(2),pt(1)/pt(2)));

			pt(0)=flines[i].x2();pt(1)=flines[i].y2();pt(2)=1.f;
			pt=M*pt;
			flines[i].setP2(QPointF(pt(0)/pt(2),pt(1)/pt(2)));
		}
	}
	// forward transfom
	void imtransform(QPointF* pts,QPointF* pts_dst,int npts,mat_33& M) // mat is the matrix, not inverse
	{
		colvec pt(3);
		for(int i=0;i<npts;++i)
		{
			pt(0)=pts[i].x();pt(1)=pts[i].y();pt(2)=1.f;
			pt=M*pt;

			pts_dst[i].setX(pt(0)/pt(2));
			pts_dst[i].setY(pt(1)/pt(2));
		}
	}

	mat_33 invH0,invH1,H0,H1,Hs,invHs;

	// call this before prewarp anything
	void prepare_prewarp(QLineF* flines0,QLineF* flines1,int lines_num)
	{
		mat_33 F=compute_F(flines0,flines1,lines_num);
// 		F <<0.0000 , -0.0000 ,   0.0001,
// 			-0.0000,    0.0000,   -0.0054,
// 			-0.0012,  0.0060,   0.3241;

		compute_H0H1(F,invH0,invH1);
		H0=invH0.inverse();H1=invH1.inverse();
	}


	void prewarp(QImage& I0,QImage& I1)
	{
		//forward_imtransform(I0,invH0);
		//forward_imtransform(I1,invH1);
		
#ifdef USE_BSGP
		backward_imtransform_bsgp(I0.bits(),I0.width(),I0.height(),I0.byteCount(),I0.bytesPerLine(),H0.data());
		backward_imtransform_bsgp(I1.bits(),I1.width(),I1.height(),I1.byteCount(),I1.bytesPerLine(),H1.data());
#else
		backward_imtransform(I0,H0);
		backward_imtransform(I1,H1);
#endif
		
	}
	void prewarp(QLineF* flines0,QLineF* flines1,int lines_num)
	{
		imtransform(flines0,lines_num,invH0);
		imtransform(flines1,lines_num,invH1);
	}
	void prewarp(QPointF* r0,QPointF* r1,QPointF* warped_r0,QPointF* warped_r1)
	{
		imtransform(r0,warped_r0,4,invH0);
		imtransform(r1,warped_r1,4,invH1);
	}

	
	// implement the cp2tform('projective') in matlab,
	// input 4 corresponding points
	mat_33 cp2tform(const mat_42& xy,const mat_42& uv)
	{
		// AX=B
		colvec4 x=xy.col(0),y=xy.col(1),
			u=uv.col(0),v=uv.col(1);
		colvec8 B;
		B.block<4,1>(0,0)=u;
		B.block<4,1>(4,0)=v;
		DBG_COUT("B: \n"<<B<<endl);
		mat_88 A;
		A.block<4,1>(0,0)=x;
		A.block<4,1>(0,1)=y;
		A.block<4,1>(0,2).fill(1);
		A.block<4,3>(0,3).fill(0);
		A.block<4,1>(0,6)=-u.cwiseProduct(x);
		A.block<4,1>(0,7)=-u.cwiseProduct(y);

		A.block<4,3>(4,0).fill(0);
		A.block<4,1>(4,3)=x;
		A.block<4,1>(4,4)=y;
		A.block<4,1>(4,5).fill(1);
		A.block<4,1>(4,6)=-v.cwiseProduct(x);
		A.block<4,1>(4,7)=-v.cwiseProduct(y);

		DBG_COUT("A: \n"<<A<<endl);

// 		assert(A.rank()>=8);
// 		if(A.rank()<8)
// 		{
// 			printf("Rank error, unable to compute the transform matrix.\n");
// 			return MatrixXd::Identity(3,3);
// 		}

		colvec X;
		X=A.colPivHouseholderQr().solve(B);

		DBG_COUT("X: \n"<<X<<endl);

		
		mat_33 T;
		T<<X(0),X(1),X(2),
		  X(3),X(4),X(5),
		  X(6),X(7),1.0;

		cout<<"T: \n"<<T<<endl;

		ele_type relative_error = (A*X - B).norm() / B.norm(); // norm() is L2 norm
		cout << "cp2tform: The relative error is " << relative_error << endl;


		return T;
	}

	// input: all QPointF* contain 4 corresponding points
	void prepare_postwarp(QPointF* r_prewarped,QPointF* r_original)
	{
		////// test
// 		r_prewarped[0]=QPointF(0,0);
// 		r_prewarped[1]=QPointF(0,512);
// 		r_prewarped[2]=QPointF(512,0);
// 		r_prewarped[3]=QPointF(512,512);
// 
// 		r_original[0]=QPointF(0.0002,-13.2715);
// 		r_original[1]=QPointF(84.1561,-536.1054);
// 		r_original[2]=QPointF(505.0365,75.1350);
// 		r_original[3]=QPointF(589.1924,-447.6989);
		//////

		mat_42 xy,uv;
		xy<<r_prewarped[0].x(),r_prewarped[0].y(),
			r_prewarped[1].x(),r_prewarped[1].y(),
			r_prewarped[2].x(),r_prewarped[2].y(),
			r_prewarped[3].x(),r_prewarped[3].y();
		uv<<r_original[0].x(),r_original[0].y(),
			r_original[1].x(),r_original[1].y(),
			r_original[2].x(),r_original[2].y(),
			r_original[3].x(),r_original[3].y();

		Hs=cp2tform(xy,uv);
		invHs=Hs.inverse();

		// test
// 		mat the_xy(4,3);
// 		the_xy.fill(1);
// 		the_xy.block<1,2>(0,0)=xy.row(0);
// 		the_xy.block<1,2>(1,0)=xy.row(1);
// 		the_xy.block<1,2>(2,0)=xy.row(2);
// 		the_xy.block<1,2>(3,0)=xy.row(3);
// 		DBG_COUT("Hs*xy: \n"<<Hs*the_xy.transpose();
	}
#define lerp(a,b,s) ((a)*(1.f-(s))+(b)*(s))
	void interpolate(QPointF* r0,QPointF* r1,float s,QPointF* rdst)
	{
		for(int i=0;i<4;++i)
		{
			rdst[i].setX(lerp(r0[i].x(),r1[i].x(),s));
			rdst[i].setY(lerp(r0[i].y(),r1[i].y(),s));
		}
	}
	void postwarp(QImage& I0)
	{
 		//forward_warp(I0,Hs);
		
#ifdef USE_BSGP
		backward_imtransform_bsgp(I0.bits(),I0.width(),I0.height(),I0.byteCount(),I0.bytesPerLine(),invHs.data());
#else
		backward_imtransform(I0,invHs);
#endif
		
	}
	void postwarp(QLineF* flines,int lines_num)
	{
		imtransform(flines,lines_num,Hs);
	}
}