uses coff,cubsgp,bsgpdbg

struct FLine
	float2 P,Q
	float2 vec, unit_perpendicular // vec/norm2 for dst image, vec for src image
	float length
		
	__device__ float weight(float2 pt,float a,b,p)
		// p: 0->1, a?, b: 0.5 -> 2
		//float p=0.5f,a=0.1f,b=1.f;
		//float a=0.5f,b=1.f,p=0.2f;
		//float a=1.f,b=1.f,p=0.5f;
		float dist=fabs(dot(pt-.P,.unit_perpendicular));
		return pow( pow(.length,p)/(a+dist) ,b);
		//return sqrt(.length)/(0.1f+dist);


objexport void warp_bsgp(
uchar* Isrc_cpu,uchar* Idst_cpu,
int w,int h,
int bytes,int bytes_line,
FLine* lines_src_cpu,
FLine* lines_dst_cpu,
int lines_num,
float a,float b,float p
)	
	Isrc=dnew uchar[bytes];
	Idst=dnew uchar[bytes];
	cpyh2d(Isrc,Isrc_cpu,sizeof(uchar)*bytes);
	
	lines_src=dnew FLine[lines_num];
	lines_dst=dnew FLine[lines_num];
	cpyh2d(lines_src,lines_src_cpu,lines_num*sizeof(FLine));
	cpyh2d(lines_dst,lines_dst_cpu,lines_num*sizeof(FLine));

	int px_num=w*h;
	forall px=0:px_num-1
		int x=px%w,y=px/w;
		
		float2 Xdst,Xsrc,D,D_sum;
		float u,v,wgt,wgt_sum;
		Xdst.x=x;Xdst.y=y;
		uchar* pdst=Idst+(y*bytes_line+x*4);
		
		D_sum.x=D_sum.y=0.f;wgt_sum=0.f;
		
		float2 PX;
		float dist;
	
		for(int ln=0;ln<lines_num;++ln)
			PX=Xdst-lines_dst[ln].P;
			u=dot(PX,lines_dst[ln].vec); // vec/norm2
			v=dot(PX,lines_dst[ln].unit_perpendicular);

			Xsrc=lines_src[ln].P+u*lines_src[ln].vec+v*lines_src[ln].unit_perpendicular;

			D=Xsrc-Xdst;
			
			if(u<0.f)
				dist=sqrt(PX.x*PX.x+PX.y*PX.y);
			else if(u>1.f)
				PX=Xdst-lines_dst[ln].Q;
				dist=sqrt(PX.x*PX.x+PX.y*PX.y);
			else
				dist=abs(v);

			wgt=pow( pow(lines_dst[ln].length,p)/(a+dist) ,b);
			
			D_sum+=D*wgt;
			wgt_sum+=wgt;

		Xsrc=Xdst+(D_sum/wgt_sum);
		
		x=(int)Xsrc.x;
		y=(int)Xsrc.y;
		/*if(x<0)
			x=0;
		if(x>=w)
			x=w-1;
		if(y<0)
			y=0;
		if(y>=h)
			y=h-1;
		uchar* psrc=Isrc+(y*bytes_line+x*4);
		*((uchar4*)pdst)=*((uchar4*)psrc);*/
		
		if(x>=0&&x<w&&y>=0&&y<h)
			uchar* psrc=Isrc+(y*bytes_line+x*4);
			*((uchar4*)pdst)=*((uchar4*)psrc);
		else
			*pdst++=0xe0;
			*pdst++=0xd0;
			*pdst++=0xc6;
			*pdst++=0xff;
		
	
	cpyd2h(Idst_cpu,Idst,sizeof(uchar)*bytes);
	
	ddelete Isrc;
	ddelete Idst;
	ddelete lines_src;
	ddelete lines_dst;
	

objexport void cross_dissolve_bsgp(
uchar* I0_cpu,uchar* I1_cpu,
int bytes,
float s,
uchar* Idst_cpu
)
	I0=dnew uchar[bytes];
	I1=dnew uchar[bytes];
	Idst=dnew uchar[bytes];
	cpyh2d(I0,I0_cpu,sizeof(uchar)*bytes);
	cpyh2d(I0,I0_cpu,sizeof(uchar)*bytes);
	
	float t=1.f-s;

	forall i=0:bytes-1
		Idst[i]=(uchar)(t*(float)I0[i]+s*(float)I1[i]);
	
	cpyd2h(Idst_cpu,Idst,sizeof(uchar)*bytes);
	
	ddelete I0;
	ddelete I0;
	ddelete Idst;

// modify the original image
// attention!! mat33 is column major
objexport void backward_imtransform_bsgp(
uchar* I0_cpu,
int w,int h,
int bytes,int bytes_line,
double* mat
)
	Isrc=dnew uchar[bytes];
	Idst=dnew uchar[bytes];
	cpyh2d(Isrc,I0_cpu,sizeof(uchar)*bytes);
	
	float[9] mat33;
	mat33[0]=(float)mat[0];
	mat33[1]=(float)mat[1];
	mat33[2]=(float)mat[2];
	mat33[3]=(float)mat[3];
	mat33[4]=(float)mat[4];
	mat33[5]=(float)mat[5];
	mat33[6]=(float)mat[6];
	mat33[7]=(float)mat[7];
	mat33[8]=(float)mat[8];
	
	// for every pixel in dst image
	int px_num=w*h;
	forall px=0:px_num-1
		int x=px%w,y=px/w;
		uchar* pdst=Idst+(y*bytes_line+x*4);
		fx=(float)x;fy=(float)y;
		
		sz=mat33[2]*fx+mat33[5]*fy+mat33[8];
		sx=(int)((mat33[0]*fx+mat33[3]*fy+mat33[6])/sz);
		sy=(int)((mat33[1]*fx+mat33[4]*fy+mat33[7])/sz);
		
		if(sx>=0&&sx<w&&sy>=0&&sy<h)
			uchar* psrc=Isrc+(sy*bytes_line+sx*4);
			*((uchar4*)pdst)=*((uchar4*)psrc);
		else
			*pdst++=0xe0;
			*pdst++=0xd0;
			*pdst++=0xc6;
			*pdst++=0xff;
		
	
	cpyd2h(I0_cpu,Idst,sizeof(uchar)*bytes);

	ddelete Isrc;
	ddelete Idst;