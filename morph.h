#ifndef MORPH_H
#define MORPH_H

class FeatureLine;
namespace beier_neely
{
	
	void morph(const QImage& I0,const QImage& I1,
		const QLineF* lines0,
		const QLineF* lines1,
		int lines_num,
		float s, // 0.0 -> 1.0
		// parameters
		float a,float b,float p,
		// output
		QImage& Idst,
		QVector<QLineF>& lines_dst,
		int show_ui);
}


#endif