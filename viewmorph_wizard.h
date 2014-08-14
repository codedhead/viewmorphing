#ifndef VIEWMORPH_WIZARD_H
#define VIEWMORPH_WIZARD_H

#include "ui_viewmorph_wizard.h"

#include <QtGui/QMainWindow>


enum VMPageId
{
	PageInput=1,
	PageLine,
	PagePrewarp,
	PagePostwarp,
	PageResult
};

#define IMAGE_MAX_WIDTH 1000
#define IMAGE_MAX_HEIGHT 1000

class ViewMorphWizard : public QWizard, private Ui::Wizard
{
	Q_OBJECT

public:
	ViewMorphWizard(QWidget *parent = 0, Qt::WFlags flags = 0);
	~ViewMorphWizard();

	int nextId () const;
	bool validateCurrentPage ();

private slots:
	void on_btn_path1_clicked();
	void on_btn_path2_clicked();

	void on_rad_path_clicked();
	void on_rad_mirror_clicked();

	void on_check_postwarp_stateChanged(int);

	void on_spin_wgt_a_valueChanged(double val);
	void on_spin_wgt_b_valueChanged(double val);
	void on_spin_wgt_p_valueChanged(double val);

	void on_show_i0_stateChanged(int);
	void on_show_i1_stateChanged(int);
	void on_show_lines_stateChanged(int);
	void on_show_prect_stateChanged(int);

	void on_slider_morph_valueChanged(int);

	void on_currentIdChanged(int id);

	void on_btn_save_images_clicked();

	void edit_postwarp(QPointF* new_rect);
private:
	void do_morph();
	void prepare_prewarp();
	void do_prewarp();

	void get_morph(QImage& mimg,QVector<QLineF>& mflines,float s,bool bpostwarp=true,QPointF* postwarp_rect=0, int show_ui=3);

	void load_flines();
	void save_flines();

	QIntValidator validator_framerate,validator_duration;

	QString file0,file1;
	QImage I0,I1;
	QVector<QLineF> flines0,flines1;
	// user specified postwarp rect
	QPointF postw_rect0[4],postw_rect1[4],
		postw_rect_half[4], // 0.5
		// prewarped version
		prewarped_postw_rect0[4],
		prewarped_postw_rect1[4];

	int last_id;

	float wgt_a,wgt_b,wgt_p,morph_frac;
};

#endif // VIEWMORPH_H
