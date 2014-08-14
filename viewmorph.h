#ifndef VIEWMORPH_H
#define VIEWMORPH_H

class QImage;
class QLineF;
class QPointF;
namespace viewmorph
{
	void prepare_prewarp(QLineF* flines1,QLineF* flines2,int lines_num);
	void prewarp(QImage& I0,QImage& I1);
	void prewarp(QLineF* flines1,QLineF* flines2,int lines_num);
	void prewarp(QPointF* r0,QPointF* r1,QPointF* warped_r0,QPointF* warped_r1);

	// input: all QPointF* contain 4 corresponding points
	void prepare_postwarp(QPointF* r0,QPointF* r1);
	void interpolate(QPointF* r0,QPointF* r1,float s,QPointF* rdst);
	void postwarp(QImage& I0);
	void postwarp(QLineF* flines,int lines_num);
}

#endif