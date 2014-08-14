#include "geometry.h"
#include "qgvimagefeature.h"
#include "morph_gpu.h"

#include <QVector>
#include <QImage>

#include <assert.h>

#include <omp.h>

#define USE_BSGP

#define EPSILON0 (1e-6)

namespace beier_neely
{
	struct CachedLine2
	{
		// norm for dst, not for src
		CachedLine2(const Point2& a,const Point2& b,bool norm_vec=false)
			:P(a),Q(b)
		{
			compute_cached(norm_vec);
		}
		CachedLine2(const QLineF& ln,bool norm_vec=false)
			:P(ln.x1(),ln.y1()),
			Q(ln.x2(),ln.y2())
		{
			compute_cached(norm_vec);
		}

		inline void compute_cached(bool norm_vec)
		{
			vec=Q-P;
			unit_perpendicular.x=-vec.y;
			unit_perpendicular.y=vec.x;

			float len2=vec.length2();
			length=sqrtf(len2);
			if(norm_vec) vec/=len2;
			unit_perpendicular/=length;
		}


		inline float weight(float dist,float a,float b,float p)const
		{
			// p: 0->1, a?, b: 0.5 -> 2
			//static float p=0.5f,a=0.1f,b=1.f;
			return powf( powf(length,p)/(a+dist) ,b);
			//return sqrtf(length)/(0.1f+dist);
		}

		Point2 P,Q;
		Vec2 vec, // divided by norm2 in dst img; not in src img
			unit_perpendicular; // normalized
		float length;

		char __ipad_04b2f294[4]; // padding, same as bsgp
	};

	QRgb bg_color=qRgb(198,208,224);

	inline QRgb bilinear_fetch(const QImage& img,float x,float y)
	{
		int ix=(int)x,iy=(int)y;
		return (ix>=0&&ix<img.width()&&iy>=0&&iy<img.height())?img.pixel((int)x,(int)y):bg_color;
	}

	void warp(const QImage& Isrc,
		const CachedLine2* lines_src,
		const CachedLine2* lines_dst,
		int lines_num,
		QImage& Idst,
		float a,float b,float p)
	{
		assert(Isrc.size()==Idst.size());

		int h=Isrc.height(),w=Isrc.width();
		uint rgba;
		float u,v;
		float dist,wgt,wgt_sum=0.f;
		Vec2 D,D_sum;

		Point2 Xdst,Xsrc;
		Vec2 PX;

		for(int y=0;y<h;++y)
		{
			for(int x=0;x<w;++x)
			{
				// every pixel in dest image
				Xdst.x=x;Xdst.y=y;

				D_sum.x=D_sum.y=0.f;wgt_sum=0.f;

				for(int ln=0;ln<lines_num;++ln)
				{
					PX=Xdst-lines_dst[ln].P;
					u=PX*lines_dst[ln].vec; // vec/norm2
					v=PX*lines_dst[ln].unit_perpendicular;

					Xsrc=lines_src[ln].P+
						u*lines_src[ln].vec+ // vec
						v*lines_src[ln].unit_perpendicular;

					D=Xsrc-Xdst;

					if(u<0.f)
						dist=PX.length();
					else if(u>1.f)
						dist=(Xdst-lines_dst[ln].Q).length();
					else
						dist=fabsf(v);

					wgt=lines_dst[ln].weight(dist,a,b,p);
					D_sum+=D*wgt;
					wgt_sum+=wgt;
				}


				
				Xsrc=Xdst+(D_sum/wgt_sum);

				rgba=bilinear_fetch(Isrc,Xsrc.x,Xsrc.y);
				Idst.setPixel(x,y,rgba);
				
				
// 				QRgb rr=Idst.pixel(x,y);
// 				//if(x>=0&&x<w&&y>=0&&y)
// 				assert((rr&(0x00ffffff))==(rgba&(0x00ffffff)));
			}
		}
	}

	/*
	void warp_incremental(const QImage& Isrc,
		const CachedLine2* lines_src,
		const CachedLine2* lines_dst,
		int lines_num,
		QImage& Idst,
		float a,float b,float p)
	{
		assert(Isrc.size()==Idst.size());

		Idst=Isrc;
		int h=Isrc.height(),w=Isrc.width();
		uint rgba;
		float u,v;
		float dist,wgt,wgt_sum=0.f;
		Vec2 D,D_sum;

		std::vector<Vec2> dXsrc_dx,dXsrc_dy;
		std::vector<Point2> Xsrc,Xsrc_scanline;
		dXsrc_dx.reserve(lines_num);dXsrc_dy.reserve(lines_num);
		Xsrc.reserve(lines_num);Xsrc_scanline.reserve(lines_num);

		Point2 Xdst,final_Xsrc;
		Vec2 PX;
		uchar* pdst=Idst.bits();

		// initial xsrc
		for(int i=0;i<lines_num;++i)
		{
			Xsrc.push_back(lines_src[i].P+
				((Point2(0.f)-lines_dst[i].P)*lines_dst[i].vec)*lines_src[i].vec+
				((Point2(0.f)-lines_dst[i].P)*lines_dst[i].unit_perpendicular)*lines_src[i].unit_perpendicular)
				;
			Xsrc_scanline.push_back(Xsrc.back());

			dXsrc_dx.push_back(
				lines_dst[i].vec.x*lines_src[i].vec
				+lines_dst[i].unit_perpendicular.x*lines_src[i].unit_perpendicular);

			dXsrc_dy.push_back(
				lines_dst[i].vec.y*lines_src[i].vec
				+lines_dst[i].unit_perpendicular.y*lines_src[i].unit_perpendicular);
		}

		for(int y=0;y<h;++y)
		{
			pdst=Idst.scanLine(y);
			for(int x=0;x<w;++x)
			{
 				Xdst.x=x;Xdst.y=y;

				D_sum.x=D_sum.y=0.f;wgt_sum=0.f;

				for(int ln=0;ln<lines_num;++ln)
				{
					D=Xsrc[ln]-Xdst;

					wgt=lines_dst[ln].weight(dist,a,b,p);
					D_sum+=D*wgt;
					wgt_sum+=wgt;

					// incre
					Xsrc[ln].x+=dXsrc_dx[ln].x;
					Xsrc[ln].y+=dXsrc_dx[ln].y;
				}
				final_Xsrc=Xdst+(D_sum/wgt_sum);

				*((QRgb*)pdst)=bilinear_fetch(Isrc,final_Xsrc.x,final_Xsrc.y);
				//Idst.setPixel(x,y,rgba);
				pdst+=4;
			}

			// incre
			for(int ln=0;ln<lines_num;++ln)
			{
				Xsrc_scanline[ln]+=dXsrc_dy[ln];
				Xsrc[ln]=Xsrc_scanline[ln];
			}
		}
	}*/

// 	inline int lerp(int a,int b,float s)
// 	{
// 		return (1-s)*a+s*b;
// 	}
#define lerp(a,b,s) ((a)*(1.f-(s))+(b)*(s))

	void cross_dissolve(const QImage& I0,const QImage& I1,float s,QImage& Idst)
	{
		assert(I0.size()==I1.size());

		int bitscnt=I0.byteCount();
		const uchar* p0=I0.bits(),*p1=I1.bits();
		uchar *pdst=Idst.bits();
		while(bitscnt--)
		{
			*pdst++=lerp(*p0++,*p1++,s);
		}
	}

	
	void morph(const QImage& I0,const QImage& I1,
		const QLineF* lines0,
		const QLineF* lines1,
		int lines_num,
		float s, // 0.0 -> 1.0
		float a,float b,float p, // parameters
		QImage& Idst,
		QVector<QLineF>& lines_dst,
		int show_ui)
	{

		assert(I0.size()==I1.size());
		

		int w=I0.width(),h=I0.height();
		QImage warped_I0(w,h,QImage::Format_ARGB32),
			warped_I1(w,h,QImage::Format_ARGB32);
		
		std::vector<CachedLine2> clines_dst,
			clines_0,clines_1;


		clines_0.reserve(lines_num);clines_1.reserve(lines_num);clines_dst.reserve(lines_num);
		
		for(int i=0;i<lines_num;++i)
		{
			clines_0.push_back(CachedLine2(lines0[i]));
			clines_1.push_back(CachedLine2(lines1[i]));
		}

		lines_dst.reserve(lines_num);
		lines_dst.clear();
		// interpolate feature lines
		//		by interpolate two endpoints
		//		or interpolate center and direction
		for(int i=0;i<lines_num;++i)
		{
			CachedLine2 cline(
				lerp(clines_0[i].P,clines_1[i].P,s),
				lerp(clines_0[i].Q,clines_1[i].Q,s),
				true
				);

			clines_dst.push_back(cline);

			lines_dst.push_back(QLineF(cline.P.x,cline.P.y,cline.Q.x,cline.Q.y));
			//lines_dst.push_back(QLineF(cline.Q.x,cline.Q.y,
			//	cline.Q.x+7.f*cline.unit_perpendicular.x,
			//	cline.Q.y+7.f*cline.unit_perpendicular.y));
		}

		int bytes=I0.byteCount(),bytes_line=I0.bytesPerLine();
#ifdef USE_BSGP
		if(show_ui&1)
			warp_bsgp((uchar*)I0.bits(),(uchar*)warped_I0.bits(),
			w,h,bytes,bytes_line,
			(FLine*)&clines_0[0],(FLine*)&clines_dst[0],lines_num,
			a,b,p);
		if(show_ui&2)
			warp_bsgp((uchar*)I1.bits(),(uchar*)warped_I1.bits(),
			w,h,bytes,bytes_line,
			(FLine*)&clines_1[0],(FLine*)&clines_dst[0],lines_num,
			a,b,p);
#else
		if(show_ui&1)
			warp(I0,&clines_0[0],&clines_dst[0],lines_num,warped_I0,a,b,p);
		if(show_ui&2)
			warp(I1,&clines_1[0],&clines_dst[0],lines_num,warped_I1,a,b,p);
		//warp_incremental(I1,&clines_1[0],&clines_dst[0],lines_num,warped_I1);
#endif


		

		//Idst=warped_I1;//.copy()
		if(show_ui==3)
#ifdef USE_BSGP
			cross_dissolve_bsgp(warped_I0.bits(),warped_I1.bits(),warped_I0.byteCount(),s,Idst.bits());
#else
			cross_dissolve(warped_I0,warped_I1,s,Idst);
#endif			
		else if(show_ui&1)
			Idst=warped_I0.copy();
		else
			Idst=warped_I1.copy();
	}
}
