#include "viewmorph_wizard.h"
#include "morph.h"
#include "viewmorph.h"
#include "geometry.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QProgressDialog>
#include <QSlider>

#include <windows.h>
#include <assert.h>


ViewMorphWizard::ViewMorphWizard(QWidget *parent, Qt::WFlags flags)
	: QWizard(parent, flags),last_id(-1),
	morph_frac(0.f),wgt_a(0.5f),wgt_b(2.f),wgt_p(0.1f),
	validator_framerate(10,60,0),validator_duration(500,5000,0)
{
	setupUi(this);
	gv_morph->the_scene.setEditable(false);
	gv_prewarp1->the_scene.setEditable(false);
	gv_prewarp2->the_scene.setEditable(false);
	spin_wgt_a->blockSignals(true);
	spin_wgt_b->blockSignals(true);
	spin_wgt_p->blockSignals(true);
	spin_wgt_a->setValue(wgt_a);
	spin_wgt_b->setValue(wgt_b);
	spin_wgt_p->setValue(wgt_p);
	spin_wgt_a->blockSignals(false);
	spin_wgt_b->blockSignals(false);
	spin_wgt_p->blockSignals(false);

	txt_duration->setValidator(&validator_duration);
	txt_framerate->setValidator(&validator_framerate);

	QObject::connect(&(gv_flines1->the_scene),SIGNAL(fline_added(FeatureLine*)),
		&(gv_flines2->the_scene),SLOT(on_fline_added(FeatureLine*)));
	QObject::connect(&(gv_flines2->the_scene),SIGNAL(fline_added(FeatureLine*)),
		&(gv_flines1->the_scene),SLOT(on_fline_added(FeatureLine*)));

	QObject::connect(&(gv_flines1->the_scene),SIGNAL(fline_removed(FeatureLine*)),
		&(gv_flines2->the_scene),SLOT(on_fline_removed(FeatureLine*)));
	QObject::connect(&(gv_flines2->the_scene),SIGNAL(fline_removed(FeatureLine*)),
		&(gv_flines1->the_scene),SLOT(on_fline_removed(FeatureLine*)));

	QObject::connect(&(gv_postwarp->the_scene),SIGNAL(prect_changed(QPointF*)),
		this,SLOT(edit_postwarp(QPointF*)));

	QObject::connect(this,SIGNAL(currentIdChanged(int)),this,SLOT(on_currentIdChanged(int)));
}

ViewMorphWizard::~ViewMorphWizard()
{
	
}

void ViewMorphWizard::on_btn_path1_clicked()
{
	QString fileName = QFileDialog::getOpenFileName(this,
		tr("Open File"), "", tr("Images (*.png *.jpg *.bmp)"));

	if(!fileName.isEmpty())
		edit_path1->setText(fileName);
}
void ViewMorphWizard::on_btn_path2_clicked()
{
	QString fileName = QFileDialog::getOpenFileName(this,
		tr("Open File"), "", tr("Images (*.png *.jpg *.bmp)"));

	if(!fileName.isEmpty())
		edit_path2->setText(fileName);
}

void ViewMorphWizard::on_rad_path_clicked()
{
	edit_path2->setEnabled(true);
	btn_path2->setEnabled(true);
}
void ViewMorphWizard::on_rad_mirror_clicked()
{
	edit_path2->setEnabled(false);
	btn_path2->setEnabled(false);
}
void ViewMorphWizard::on_check_postwarp_stateChanged(int state)
{
	gv_flines1->the_scene.showPRect(state==Qt::Checked);
	gv_flines2->the_scene.showPRect(state==Qt::Checked);
}

void ViewMorphWizard::on_slider_morph_valueChanged(int v)
{
	morph_frac=((float)v)/((float)slider_morph->maximum());
	slider_text_morph->setText(QString::number(morph_frac));
	do_morph();
}
void ViewMorphWizard::on_spin_wgt_a_valueChanged(double val)
{
	wgt_a=val;
	do_morph();
}
void ViewMorphWizard::on_spin_wgt_b_valueChanged(double val)
{
	wgt_b=val;
	do_morph();
}
void ViewMorphWizard::on_spin_wgt_p_valueChanged(double val)
{
	wgt_p=val;
	do_morph();
}
void ViewMorphWizard::on_show_i0_stateChanged(int)
{
	do_morph();
}
void ViewMorphWizard::on_show_i1_stateChanged(int)
{
	do_morph();
}
void ViewMorphWizard::on_show_lines_stateChanged(int)
{
	gv_morph->the_scene.showLines(show_lines->isChecked());
}
void ViewMorphWizard::on_show_prect_stateChanged(int)
{
	gv_morph->the_scene.showPRect(show_prect->isChecked());
}

void ViewMorphWizard::on_btn_save_images_clicked()
{
	QString name_prefix=QFileDialog::getSaveFileName(this,
		tr("Save File"),"",tr("Images (*.png)"));
	if(name_prefix.isEmpty()) return;

	name_prefix.remove(name_prefix.lastIndexOf(".png"),4);

	bool ok1,ok2;
	int pics=ceilf(txt_framerate->text().toInt(&ok1)*
		(txt_duration->text().toFloat(&ok2)/1000.f));

	if(!ok1||!ok2) return;
	
	QImage mimg(I0.width(),I0.height(),QImage::Format_ARGB32);
	QVector<QLineF> mflines;
	float invp=1.f/pics;
	QString s_filename;
	char buf[32];

	QProgressDialog progress("Saving images...", "Abort", 0, pics, this);
	progress.setWindowModality(Qt::WindowModal);
	
	bool bpostwarp=check_postwarp->isChecked();
	for(int i=0;i<pics;++i)
	{
		progress.setValue(i);
		if (progress.wasCanceled())
			break;

		float s=min(invp*i,1.f);

		get_morph(mimg,mflines,s,bpostwarp);

		sprintf(buf,"_%03d.png",i);
		s_filename=name_prefix;
		s_filename.append(buf);
		mimg.save(s_filename);
	}
	progress.setValue(pics);
}
int ViewMorphWizard::nextId () const
{
	switch(this->currentId())
	{
	case PageLine:
		if(check_prewarp->isChecked())
			return PagePrewarp;
		else if(check_postwarp->isChecked())
			return PagePostwarp;
		return PageResult;
		break;
	case PagePrewarp:
		if(!check_postwarp->isChecked())
			return PageResult;
		break;
	}
	return currentPage()->nextId();
}

void ViewMorphWizard::load_flines()
{
	QString f_flines0=file0+".flines",
		 f_flines1=file1+".flines";
	flines0.clear();
	flines1.clear();
	postw_rect1[0]=postw_rect0[0]=QPointF(0.25f*I0.width(),0.25f*I0.height());
	postw_rect1[1]=postw_rect0[1]=QPointF(0.75f*I0.width(),0.25f*I0.height());
	postw_rect1[2]=postw_rect0[2]=QPointF(0.75f*I0.width(),0.75f*I0.height());
	postw_rect1[3]=postw_rect0[3]=QPointF(0.25f*I0.width(),0.75f*I0.height());

	float fbuf[8]; // x1,y1,x2,y2
	QPointF postw_rc[4];

	FILE* fp=fopen(f_flines0.toStdString().c_str(),"rt");
	if(fp)
	{
		int fline_num;
		if(fscanf(fp,"%d",&fline_num)==1)
		{
			for(int i=0;i<fline_num;++i)
			{
				if(fscanf(fp,"%f%f%f%f",fbuf,fbuf+1,fbuf+2,fbuf+3)!=4)
					break;

				flines0.push_back(QLineF(fbuf[0],fbuf[1],fbuf[2],fbuf[3]));
			}

		}
		if(fscanf(fp,"%f%f%f%f%f%f%f%f",
			fbuf,fbuf+1,fbuf+2,fbuf+3,fbuf+4,fbuf+5,fbuf+6,fbuf+7)==8)
		{
			postw_rect0[0].setX(fbuf[0]);
			postw_rect0[0].setY(fbuf[1]);
			postw_rect0[1].setX(fbuf[2]);
			postw_rect0[1].setY(fbuf[3]);
			postw_rect0[2].setX(fbuf[4]);
			postw_rect0[2].setY(fbuf[5]);
			postw_rect0[3].setX(fbuf[6]);
			postw_rect0[3].setY(fbuf[7]);
		}

		fclose(fp);
	}
	fp=fopen(f_flines1.toStdString().c_str(),"rt");
	if(fp)
	{
		int fline_num;
		if(fscanf(fp,"%d",&fline_num)==1)
		{
			for(int i=0;i<fline_num;++i)
			{
				if(fscanf(fp,"%f%f%f%f",fbuf,fbuf+1,fbuf+2,fbuf+3)!=4)
					break;

				flines1.push_back(QLineF(fbuf[0],fbuf[1],fbuf[2],fbuf[3]));
			}

		}
		if(fscanf(fp,"%f%f%f%f%f%f%f%f",
			fbuf,fbuf+1,fbuf+2,fbuf+3,fbuf+4,fbuf+5,fbuf+6,fbuf+7)==8)
		{
			postw_rect1[0].setX(fbuf[0]);
			postw_rect1[0].setY(fbuf[1]);
			postw_rect1[1].setX(fbuf[2]);
			postw_rect1[1].setY(fbuf[3]);
			postw_rect1[2].setX(fbuf[4]);
			postw_rect1[2].setY(fbuf[5]);
			postw_rect1[3].setX(fbuf[6]);
			postw_rect1[3].setY(fbuf[7]);
		}

		fclose(fp);
	}

	if(flines0.size()<flines1.size())
	{
		for(int i=flines0.size(),iLen=flines1.size();i<iLen;++i)
			flines0.push_back(flines1[i]);
	}
	else if(flines1.size()<flines0.size())
	{
		for(int i=flines1.size(),iLen=flines0.size();i<iLen;++i)
			flines1.push_back(flines0[i]);
	}

	gv_flines1->the_scene.setImage(I0);
	gv_flines2->the_scene.setImage(I1);

	// add and pair lines
	FeatureLine* f0,*f1;
	for(int i=0,iLen=flines0.size();i<iLen;++i)
	{
		f0=new FeatureLine(flines0[i]);
		f1=new FeatureLine(flines1[i]);
		f0->setPair(f1);
		f1->setPair(f0);

		gv_flines1->the_scene.addItem(f0);
		gv_flines2->the_scene.addItem(f1);
	}
	PostwRect* r0,*r1;
	r0=new PostwRect(postw_rect0,true);
	r1=new PostwRect(postw_rect1,true);
	r0->setPair(r1);r1->setPair(r0);
	r0->setVisible(check_postwarp->isChecked());
	r1->setVisible(check_postwarp->isChecked());
	gv_flines1->the_scene.addItem(r0);
	gv_flines2->the_scene.addItem(r1);

}
void ViewMorphWizard::save_flines()
{
	gv_flines1->the_scene.saveLines(flines0,flines1,postw_rect0,postw_rect1);
	// or
	// gv_flines2->the_scene.saveLines(flines1,flines0);

	assert(flines0.size()==flines1.size());

	QString f_flines0=file0+".flines",
		f_flines1=file1+".flines";

	FILE* fp=fopen(f_flines0.toStdString().c_str(),"wt");
	if(fp)
	{
		fprintf(fp,"%d\n",flines0.size());
		for(int i=0,iLen=flines0.size();i<iLen;++i)
		{
// 			rline[0]=flines0[i].x1();
// 			rline[1]=flines0[i].y1();
// 			rline[2]=flines0[i].x2();
// 			rline[3]=flines0[i].y2();
// 			fwrite(rline,4*sizeof(float),1,fp);
			fprintf(fp,"%f %f\n%f %f\n"
				,flines0[i].x1()
				,flines0[i].y1()
				,flines0[i].x2()
				,flines0[i].y2());
		}
		fprintf(fp,"%f %f\n%f %f\n%f %f\n%f %f\n",
			postw_rect0[0].x(),postw_rect0[0].y(),
			postw_rect0[1].x(),postw_rect0[1].y(),
			postw_rect0[2].x(),postw_rect0[2].y(),
			postw_rect0[3].x(),postw_rect0[3].y());
		fclose(fp);
	}
	fp=fopen(f_flines1.toStdString().c_str(),"wt");
	if(fp)
	{
		fprintf(fp,"%d\n",flines1.size());
		for(int i=0,iLen=flines1.size();i<iLen;++i)
		{
			fprintf(fp,"%f %f\n%f %f\n"
				,flines1[i].x1()
				,flines1[i].y1()
				,flines1[i].x2()
				,flines1[i].y2());
		}
		fprintf(fp,"%f %f\n%f %f\n%f %f\n%f %f\n",
			postw_rect1[0].x(),postw_rect1[0].y(),
			postw_rect1[1].x(),postw_rect1[1].y(),
			postw_rect1[2].x(),postw_rect1[2].y(),
			postw_rect1[3].x(),postw_rect1[3].y());
		fclose(fp);
	}
}

bool ViewMorphWizard::validateCurrentPage ()
{
	switch(this->currentId())
	{
	case PageLine:
		gv_flines1->the_scene.saveLines(flines0,flines1,postw_rect0,postw_rect1);
		// or
		// gv_flines2->the_scene.saveLines(flines1,flines0);

		if(flines0.size()!=flines1.size())
		{
			QMessageBox::warning(this,"Error","Feature lines number don't match.");
			return false;
		}
		if(flines0.size()<1)
		{
			QMessageBox::warning(this,"Error","At least 4 feature lines are needed.");
			return false;
		}
		break;
	case PageInput:
		if(rad_path->isChecked())
		{
			if(!I0.load(edit_path1->text())||!I1.load(edit_path2->text()))
			{
				QMessageBox::warning(this,"Error","Failed to load image file.");
				return false;
			}

			if(I0.height()!=I1.height()||I0.width()!=I1.width()||
				I1.height()>IMAGE_MAX_HEIGHT||I1.width()>IMAGE_MAX_WIDTH)
			{
				I0.load(0,0);I1.load(0,0);
				QMessageBox::warning(this,"Error","Image size not match or too large.");
				return false;
			}

			file0=edit_path1->text();
			file1=edit_path2->text();
		}
		else
		{
			if(!I0.load(edit_path1->text()))
			{
				QMessageBox::warning(this,"Error","Failed to load image file.");
				return false;
			}

			if(I0.height()>IMAGE_MAX_HEIGHT||I0.width()>IMAGE_MAX_WIDTH)
			{
				I0.load(0,0);
				QMessageBox::warning(this,"Error","Image is too large.");
				return false;
			}

			I1=I0.mirrored(true,false);

			file0=edit_path1->text();
			file1=edit_path1->text()+".mirror";
		}

		break;
	case PageResult:
		return false;
		break;
	}
	return true;
}
void ViewMorphWizard::prepare_prewarp()
{
	// compute F,H0,H1
	viewmorph::prepare_prewarp(flines0.data(),flines1.data(),flines0.size());
	// always prewarp postwarp-rectangle, this might be need for calculating Hs even when prewarp is not enabled
	viewmorph::prewarp(postw_rect0,postw_rect1,prewarped_postw_rect0,prewarped_postw_rect1);
}

void ViewMorphWizard::do_prewarp()
{
	// prewarp image, feature lines
	viewmorph::prewarp(I0,I1);
	viewmorph::prewarp(flines0.data(),flines1.data(),flines0.size());

	gv_prewarp1->the_scene.setImageLines(I0,flines0,check_postwarp->isChecked()?prewarped_postw_rect0:0);
	gv_prewarp2->the_scene.setImageLines(I1,flines1,check_postwarp->isChecked()?prewarped_postw_rect1:0);
}
LARGE_INTEGER freq;
void ViewMorphWizard::do_morph()
{
	slider_morph->blockSignals(true);

	static bool _get_freq=true;
	if(_get_freq)
	{
		_get_freq=false;
		QueryPerformanceFrequency(&freq);
	}
	LARGE_INTEGER pc1,pc2;
	QueryPerformanceCounter(&pc1);



	QImage mimg(I0.width(),I0.height(),QImage::Format_ARGB32);
	QVector<QLineF> mflines;
	assert(flines0.size()==flines1.size());
	int show_ui=0;
	show_ui=show_i1->isChecked();
	show_ui<<=1;
	show_ui|=show_i0->isChecked();

	QPointF postwarp_rect[4];
	get_morph(mimg,mflines,morph_frac,check_postwarp->isChecked(),postwarp_rect,show_ui);

	gv_morph->the_scene.showLines(show_lines->isChecked());
	gv_morph->the_scene.showPRect(check_postwarp->isChecked()&&show_prect->isChecked());
	gv_morph->the_scene.setImageLines(mimg,mflines,postwarp_rect,false);



	QueryPerformanceCounter(&pc2);
	double timecnt=((double)(pc2.QuadPart-pc1.QuadPart))/((double)freq.QuadPart);
	printf("morph in %f ms\n",1000.*timecnt);

	slider_morph->blockSignals(false);
}

void ViewMorphWizard::get_morph(QImage& mimg,QVector<QLineF>& mflines,float s,bool bpostwarp,QPointF* postwarp_rect, int show_ui)
{
	assert(flines0.size()==flines1.size());

	beier_neely::morph(I0,I1,flines0.data(),flines1.data(),flines0.size(),
		s,wgt_a,wgt_b,wgt_p,
		mimg,mflines,show_ui);

	if(bpostwarp)
	{
		QPointF local_postwarp_rect[4],prewarped_postwarp_rect[4];
		if(!postwarp_rect)	postwarp_rect=local_postwarp_rect;

		if(s<0.5f)
			viewmorph::interpolate(postw_rect0,postw_rect_half,s/0.5f,postwarp_rect);
		else
			viewmorph::interpolate(postw_rect_half,postw_rect1,(s-0.5f)/0.5f,postwarp_rect);

		viewmorph::interpolate(prewarped_postw_rect0,prewarped_postw_rect1,s,prewarped_postwarp_rect);
		viewmorph::prepare_postwarp(prewarped_postwarp_rect,postwarp_rect);

		viewmorph::postwarp(mimg);
		viewmorph::postwarp(mflines.data(),mflines.size());
	}
}
void ViewMorphWizard::edit_postwarp(QPointF* new_rect)
{
	// this function is called when user has editted the postw_rect_half

	if(new_rect)
	{
		postw_rect_half[0]=new_rect[0];
		postw_rect_half[1]=new_rect[1];
		postw_rect_half[2]=new_rect[2];
		postw_rect_half[3]=new_rect[3];
	}

	QImage mimg(I0.width(),I0.height(),QImage::Format_ARGB32);
	QVector<QLineF> mflines;

	get_morph(mimg,mflines,0.5f);

	gv_postwarp->the_scene.setImage(mimg,false);
}
void ViewMorphWizard::on_currentIdChanged(int id)
{
	switch(last_id)
	{
	case PageLine:
		save_flines();
		if(id>last_id) prepare_prewarp();
		break;
	}
	switch(id)
	{
	case PageLine:
		if(last_id==PagePrewarp)
		{
			I0.load(file0);
			if(rad_path->isChecked())
				I1.load(file1);
			else
				I1=I0.mirrored(true,false);
		}
		load_flines();
		break;
	case PagePrewarp:
		if(last_id==PageLine)
		{
			if(check_prewarp->isChecked())
				do_prewarp();
		}
		break;
	case PagePostwarp:
		// if we are from previous step, reset the half rect
		if(last_id<id)
			viewmorph::interpolate(postw_rect0,postw_rect1,0.5f,postw_rect_half);
		gv_postwarp->the_scene.setImageRect(I0,postw_rect_half);
		// update the image for the first time
		edit_postwarp(0);
		break;
	case PageResult:
		slider_morph->setValue(0);
		gv_morph->the_scene.setImageLines(I0,flines0,postw_rect0);
		slider_morph->blockSignals(true);
		do_morph();
		slider_morph->blockSignals(false);
		break;
	}

	last_id=id;
}