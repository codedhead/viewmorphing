#ifndef QGV_IMAGE_FEATURE_H
#define QGV_IMAGE_FEATURE_H

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsItem>

enum GraphicsItemType{TYPE_FLINE=QGraphicsItem::UserType+1,TYPE_PRECT};
enum DragLinePos{DRAG_NONE,DRAG_HEAD,DRAG_ARROW,DRAG_MIDDLE};

class PostwRect: public QGraphicsObject
{
	Q_OBJECT
public:
	enum { Type = TYPE_PRECT };
	int type() const { return Type; }

	PostwRect();
	PostwRect(const QPointF* pts,bool edable=true);

	int where(const QPointF& mp);

	QRectF boundingRect () const;
	QPainterPath shape () const;
	void paint ( QPainter * painter, 
		const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );

	void setP(const QPointF* pts){p[0]=pts[0];p[1]=pts[1];p[2]=pts[2];p[3]=pts[3];}
	void setPair(PostwRect* p){pair=p;}

	void highlight(int i){highlight_pi=i;update();}
	void unhighlight()
	{
		if(highlight_pi>=0&&highlight_pi<4)
		{
			highlight_pi=-1;update();
		}
	}

signals:
	void prect_changed(QPointF*);

protected:
	void mouseMoveEvent ( QGraphicsSceneMouseEvent * event );
	void mousePressEvent ( QGraphicsSceneMouseEvent * event );
	void mouseReleaseEvent ( QGraphicsSceneMouseEvent * event );
	void hoverEnterEvent ( QGraphicsSceneHoverEvent * event );
	void hoverLeaveEvent ( QGraphicsSceneHoverEvent * event );
private:
	bool editable;
	int drag_pi,highlight_pi;
public:
	QPointF p[4];
	PostwRect* pair;
};

class FeatureLine: public QGraphicsItem
{
public:
	enum { Type = TYPE_FLINE };
	int type() const { return Type; }

	FeatureLine();
	FeatureLine(const FeatureLine* ln,bool edable=true);
	FeatureLine(const QLineF& ln,bool edable=true);

	DragLinePos where(const QPointF& mp);

	QRectF boundingRect () const;
	QPainterPath shape () const;
	void paint ( QPainter * painter, 
		const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );

	void setP1P2(const QPointF& pt1,const QPointF& pt2);
	void setP2(const QPointF& pt2);
	void setPair(FeatureLine* p){pair=p;}

	void highlight(){hlighted=true;update();}
	void unhighlight(){hlighted=false;update();}

protected:
	void mouseMoveEvent ( QGraphicsSceneMouseEvent * event );
	void mousePressEvent ( QGraphicsSceneMouseEvent * event );
	void mouseReleaseEvent ( QGraphicsSceneMouseEvent * event );
	void hoverEnterEvent ( QGraphicsSceneHoverEvent * event );
	void hoverLeaveEvent ( QGraphicsSceneHoverEvent * event );
private:
	DragLinePos drag_pos;
	bool hlighted;
	bool editable;
public:
	QPointF p1,p2;
	FeatureLine* pair;
};
class QGVImageFeature;
class ImageFeatureScene:public QGraphicsScene
{
	Q_OBJECT
public:
	ImageFeatureScene(QGVImageFeature* parent);
	~ImageFeatureScene();

	void setEditable(bool e){editable=e;}

	// for lines editing
	void setImage(const QImage& img,bool clean_items=true);
	void saveLines(QVector<QLineF>& lns_me,QVector<QLineF>& lns_pair,QPointF* prect_me,QPointF* prect_pair);

	void showLines(bool vis);
	void showPRect(bool vis);

	// for displaying results
	void setImageLines(const QImage& img,const QVector<QLineF>& lns,QPointF* prect,bool re_create=true);
	// for displaying postwarp editting, bind signals
	void setImageRect(const QImage& img,QPointF* prect);

signals:
	void fline_added(FeatureLine*);
	void fline_removed(FeatureLine*);
	void prect_changed(QPointF*);

public slots:
	void on_fline_added(FeatureLine*);
	void on_fline_removed(FeatureLine*);

protected:
	void drawBackground ( QPainter * painter, const QRectF & rect );
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
	void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private:
	FeatureLine line_ghost;
	bool dragging;
	bool editable;
	QImage bkg_img;
};

class QGVImageFeature: public QGraphicsView
{
	Q_OBJECT
public:
	QGVImageFeature(/*bool addable,*/QWidget *parent=0);

	ImageFeatureScene the_scene;
};

#endif