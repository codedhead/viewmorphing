#include "qgvimagefeature.h"

#include <QGraphicsSceneMouseEvent>
#include <QVector2D>
#include <QPolygon>

#include <assert.h>
#include <math.h>

inline float min(float a,float b)
{
	return a<b?a:b;
}
inline float min(float a,float b,float c,float d)
{
	if(a<b)
	{
		if(a<c)
		{
			return a<d?a:d;
		}
		else
		{
			return c<d?c:d;
		}
	}
	else
	{
		if(b<c)
		{
			return b<d?b:d;
		}
		else
		{
			return c<d?c:d;
		}
	}
}
inline float max(float a,float b,float c,float d)
{
	if(a>b)
	{
		if(a>c)
		{
			return a>d?a:d;
		}
		else
		{
			return c>d?c:d;
		}
	}
	else
	{
		if(b>c)
		{
			return b>d?b:d;
		}
		else
		{
			return c>d?c:d;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// FeatureLine

FeatureLine::FeatureLine()
:pair(0),drag_pos(DRAG_NONE),hlighted(false),editable(true)
{
	this->setAcceptHoverEvents(editable);
}
FeatureLine::FeatureLine(const QLineF& ln,bool edable)
:pair(0),drag_pos(DRAG_NONE),hlighted(false),editable(edable)
{
	this->setAcceptHoverEvents(editable);
	this->p1=ln.p1();
	this->p2=ln.p2();
}
FeatureLine::FeatureLine(const FeatureLine* ln,bool edable)
:pair(0),drag_pos(DRAG_NONE),hlighted(false),editable(edable)
{
	this->setAcceptHoverEvents(editable);
	this->p1=ln->p1;
	this->p2=ln->p2;
}

void FeatureLine::paint ( QPainter * painter, 
			const QStyleOptionGraphicsItem * option, QWidget * widget)
{
	if(hlighted)
	{
		painter->setPen(QPen(Qt::blue, 1.3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
		painter->setBrush(QBrush(Qt::blue));
	}
	else
	{
		painter->setPen(QPen(Qt::red, 1.3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
		painter->setBrush(QBrush(Qt::red));
	}
	
	painter->drawLine(p1,p2);

	QPolygonF arrow;
#define ARROW_WIDTH (3)
#define ARROW_LENGTH (11)

	QVector2D ortho(p1.y()-p2.y(),p2.x()-p1.x());
	ortho.normalize();
	ortho*=ARROW_WIDTH;
	QPointF ofst(ortho.x(),ortho.y());

	QPointF p1p2(p2-p1);
	p1p2/=sqrtf(p1p2.x()*p1p2.x()+p1p2.y()*p1p2.y());

	QPointF q=p2-(p1p2*ARROW_LENGTH);

	arrow.append(p2);
	arrow.append(q-ofst);
	arrow.append(q+ofst);

	painter->drawPolygon(arrow);

	
	//painter->drawPath(shape());
	//painter->drawRect(boundingRect());
}
QRectF FeatureLine::boundingRect () const
{
	QRectF rc(min(p1.x(),p2.x()),min(p1.y(),p2.y()),fabs(p1.x()-p2.x()),fabs(p1.y()-p2.y()));
#define BRECT_OFFSET (4)
	rc.adjust(-BRECT_OFFSET,-BRECT_OFFSET,BRECT_OFFSET,BRECT_OFFSET);
	return rc;
}

QPolygonF line_shape(const QPointF& p1,const QPointF& p2,float half_width)
{
	QPolygonF ply;

	QVector2D ortho(p1.y()-p2.y(),p2.x()-p1.x());
	ortho.normalize();
	ortho*=half_width;
	QPointF ofst(ortho.x(),ortho.y());

	QPointF p1p2(p2-p1);
	p1p2/=sqrtf(p1p2.x()*p1p2.x()+p1p2.y()*p1p2.y());
	QPointF q1=p1-half_width*p1p2,q2=p2+half_width*p1p2;

	ply.append(q1+ofst);
	ply.append(q1-ofst);
	ply.append(q2-ofst);
	ply.append(q2+ofst);

	return ply;
}

QPainterPath FeatureLine::shape () const
{
	QPainterPath path;
	if(p1==p2) return path;

#define SHAPE_WIDTH (4)
	
	path.addPolygon(line_shape(p1,p2,SHAPE_WIDTH));
	return path;
}

DragLinePos FeatureLine::where(const QPointF& mp)
{
	QVector2D p1p2(p2-p1),
		p1mp(mp-p1);
	qreal len=p1p2.length();p1p2.normalize();
	qreal r=QVector2D::dotProduct(p1mp,p1p2)/len;

	if(r<0.1f) return DRAG_HEAD;
	else if(r<=1.f&&(1-r)*len<=ARROW_LENGTH) return DRAG_ARROW;
	else if(r>=0.3f&&r<=0.7f) return DRAG_MIDDLE;
	return DRAG_NONE;
}

void FeatureLine::setP1P2(const QPointF& pt1,const QPointF& pt2)
{
	prepareGeometryChange();
	p1=pt1;p2=pt2;
	update();
}
void FeatureLine::setP2(const QPointF& pt2)
{
	prepareGeometryChange();
	p2=pt2;
	update();
}

void FeatureLine::mouseMoveEvent ( QGraphicsSceneMouseEvent * event )
{
	if(!editable)
		return QGraphicsItem::mouseMoveEvent(event);

	QPointF dp=event->pos()-event->lastPos();;
	prepareGeometryChange();
	switch(drag_pos)
	{
	case DRAG_ARROW:
		p2+=dp;
		break;
	case DRAG_HEAD:
		p1+=dp;
		break;
	case DRAG_MIDDLE:
		p1+=dp;
		p2+=dp;
		break;
	default:
		return QGraphicsItem::mouseMoveEvent(event);
	}
	update();
	event->accept();
}
void FeatureLine::mousePressEvent ( QGraphicsSceneMouseEvent * event )
{
	if(!editable)
		return QGraphicsItem::mousePressEvent(event);

	drag_pos=DRAG_NONE;
	if(event->button()==Qt::LeftButton)
	{
		drag_pos=where(event->pos());
		return event->accept();
	}
	return QGraphicsItem::mousePressEvent(event);
}
void FeatureLine::mouseReleaseEvent ( QGraphicsSceneMouseEvent * event )
{
	if(!editable)
		return QGraphicsItem::mouseReleaseEvent(event);

	drag_pos=DRAG_NONE;
	return QGraphicsItem::mouseReleaseEvent(event);
}
void FeatureLine::hoverEnterEvent ( QGraphicsSceneHoverEvent * event )
{
	if(!editable)
		return QGraphicsItem::hoverEnterEvent(event);

	highlight();
	if(pair) pair->highlight();
	return QGraphicsItem::hoverEnterEvent(event);
}
void FeatureLine::hoverLeaveEvent ( QGraphicsSceneHoverEvent * event )
{
	if(!editable)
		return QGraphicsItem::hoverLeaveEvent(event);

	unhighlight();
	if(pair) pair->unhighlight();
	return QGraphicsItem::hoverLeaveEvent(event);
}

//////////////////////////////////////////////////////////////////////////
// PostwRect

#define PRECT_BOUND_OFFSET (4)
#define PRECT_POINT_SIZE (4)

PostwRect::PostwRect()
:drag_pi(-1),editable(true),highlight_pi(-1),pair(0)
{
	this->setAcceptHoverEvents(editable);
}
PostwRect::PostwRect(const QPointF* pts,bool edable)
:drag_pi(-1),editable(edable),highlight_pi(-1),pair(0)
{
	p[0]=pts[0];
	p[1]=pts[1];
	p[2]=pts[2];
	p[3]=pts[3];
	this->setAcceptHoverEvents(editable);
}

int PostwRect::where(const QPointF& mp)
{
	for(int i=0;i<4;++i)
	{
		if(QVector2D(mp-p[i]).lengthSquared()<=PRECT_POINT_SIZE*PRECT_POINT_SIZE)
			return i;
	}
	return -1;
}

QRectF PostwRect::boundingRect () const
{
	QPointF pmin(min(p[0].x(),p[1].x(),p[2].x(),p[3].x()),
					min(p[0].y(),p[1].y(),p[2].y(),p[3].y())),
			pmax(max(p[0].x(),p[1].x(),p[2].x(),p[3].x()),
					max(p[0].y(),p[1].y(),p[2].y(),p[3].y()));

	QRectF rc(pmin,pmax);

	rc.adjust(-PRECT_BOUND_OFFSET,-PRECT_BOUND_OFFSET,PRECT_BOUND_OFFSET,PRECT_BOUND_OFFSET);

	return rc;
}
QPainterPath PostwRect::shape () const
{
	QPainterPath path;

	path.addEllipse(p[0],PRECT_POINT_SIZE,PRECT_POINT_SIZE);
	path.addEllipse(p[1],PRECT_POINT_SIZE,PRECT_POINT_SIZE);
	path.addEllipse(p[2],PRECT_POINT_SIZE,PRECT_POINT_SIZE);
	path.addEllipse(p[3],PRECT_POINT_SIZE,PRECT_POINT_SIZE);

	return path;
}
void PostwRect::paint ( QPainter * painter, 
			const QStyleOptionGraphicsItem * option, QWidget * widget )
{
	QPolygonF ply;
	ply.append(p[0]);ply.append(p[1]);ply.append(p[2]);ply.append(p[3]);ply.append(p[0]);

	painter->setPen(QPen(Qt::white, 1.3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
	painter->setBrush(QBrush(Qt::white));
	painter->drawPolyline(ply);
	for(int i=0;i<4;++i)
	{
		if(i==highlight_pi)
		{
			painter->setPen(QPen(Qt::black, 1.3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
			painter->setBrush(QBrush(Qt::black));
		}
		else
		{
			painter->setPen(QPen(Qt::white, 1.3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
			painter->setBrush(QBrush(Qt::white));
		}
		painter->drawEllipse(p[i],PRECT_POINT_SIZE,PRECT_POINT_SIZE);
	}
}

void PostwRect::mouseMoveEvent ( QGraphicsSceneMouseEvent * event )
{
	if(!editable||drag_pi<0||drag_pi>=4)
		return QGraphicsItem::mouseMoveEvent(event);

	QPointF dp=event->pos()-event->lastPos();;
	prepareGeometryChange();
	p[drag_pi]+=dp;
	update();
	event->accept();
}
void PostwRect::mousePressEvent ( QGraphicsSceneMouseEvent * event )
{
	if(!editable)
		return QGraphicsItem::mousePressEvent(event);

	drag_pi=-1;
	if(event->button()==Qt::LeftButton)
	{
		drag_pi=where(event->pos());
		if(drag_pi>=0)
			return event->accept();
	}
	return QGraphicsItem::mousePressEvent(event);
}
void PostwRect::mouseReleaseEvent ( QGraphicsSceneMouseEvent * event )
{
	if(!editable)
		return QGraphicsItem::mouseReleaseEvent(event);

	if(drag_pi>=0&&drag_pi<4)
		emit prect_changed(p);
	drag_pi=-1;
	return QGraphicsItem::mouseReleaseEvent(event);
}
void PostwRect::hoverEnterEvent ( QGraphicsSceneHoverEvent * event )
{
	if(!editable)
		return QGraphicsItem::hoverEnterEvent(event);

	int i=where(event->pos());
	highlight(i);
	if(pair) pair->highlight(i);
	return QGraphicsItem::hoverEnterEvent(event);
}
void PostwRect::hoverLeaveEvent ( QGraphicsSceneHoverEvent * event )
{
	if(!editable)
		return QGraphicsItem::hoverLeaveEvent(event);

	unhighlight();
	if(pair) pair->unhighlight();
	return QGraphicsItem::hoverLeaveEvent(event);
}

//////////////////////////////////////////////////////////////////////////
// ImageFeatureScene

ImageFeatureScene::ImageFeatureScene(QGVImageFeature* parent)
:QGraphicsScene(parent),editable(true)
{
	line_ghost.setVisible(false);
	this->addItem(&line_ghost);
	dragging=false;

	this->setBackgroundBrush(Qt::blue);
}

ImageFeatureScene::~ImageFeatureScene()
{
	this->removeItem(&line_ghost);
}
void ImageFeatureScene::drawBackground ( QPainter * painter, const QRectF & rect )
{
	painter->drawImage(0,0,bkg_img);
}
void ImageFeatureScene::setImage(const QImage& img,bool clean_items)
{
	//QBrush bkg(img);
	//this->setBackgroundBrush(bkg);
	bkg_img=img;
	if(clean_items)
	{
		this->removeItem(&line_ghost);
		this->clear(); // memory?

		line_ghost.setVisible(false);
		this->addItem(&line_ghost);
	}	

	int h=img.height(),w=img.width();
	this->setSceneRect(0,0,w,h);

	update();
}
void ImageFeatureScene::setImageRect(const QImage& img,QPointF* prect)
{
	bkg_img=img;

	int h=img.height(),w=img.width();
	this->setSceneRect(0,0,w,h);

	this->removeItem(&line_ghost);
	this->clear(); // memory?

	PostwRect* newrect=new PostwRect(prect,true);
	this->addItem(newrect);

 	QObject::connect(newrect,SIGNAL(prect_changed(QPointF*)),
 		this,SIGNAL(prect_changed(QPointF*)));
	
	update();
}

void ImageFeatureScene::setImageLines(const QImage& img,const QVector<QLineF>& lns,QPointF* prect,bool re_create)
{
	//QBrush bkg(img);
	//this->setBackgroundBrush(bkg);
	bkg_img=img;

	int h=img.height(),w=img.width();
	this->setSceneRect(0,0,w,h);

	if(re_create)
	{
		this->removeItem(&line_ghost);
		this->clear(); // memory?
		//line_ghost.setVisible(false);
		//this->addItem(&line_ghost);

		for(int i=0,iLen=lns.size();i<iLen;++i)
		{
			this->addItem(new FeatureLine(lns[i],editable));
		}
		if(prect) this->addItem(new PostwRect(prect,false));
	}
	else
	{
		QList<QGraphicsItem*> gitems=items();
//		assert(gitems.size()==lns.size()+1); // lines and rect
// 		if(gitems.size()!=lns.size()+1)
// 		{
// 			printf("morph results, flines not match\n");
// 			return;
// 		}

		for(int i=0,j=0,iLen=gitems.size();i<iLen;++i)
		{
			if(gitems[i]->type()==TYPE_FLINE)
			{
				((FeatureLine*)gitems[i])->setP1P2(lns[j].p1(),lns[j].p2());
				++j;
			}
			else if(gitems[i]->type()==TYPE_PRECT&&prect)
			{
				((PostwRect*)gitems[i])->setP(prect);
			}
		}
	}
	update();
}

void ImageFeatureScene::saveLines(QVector<QLineF>& lns_me,QVector<QLineF>& lns_pair,QPointF* prect_me,QPointF* prect_pair)
{
	lns_me.clear();lns_pair.clear();
	QList<QGraphicsItem*> gitems=this->items();
	FeatureLine* litem;
	foreach(QGraphicsItem* gitem, gitems)
	{
		if(gitem->type()==TYPE_PRECT)
		{
			QPointF* pts=((PostwRect*)gitem)->p;
			prect_me[0]=pts[0];prect_me[1]=pts[1];prect_me[2]=pts[2];prect_me[3]=pts[3];
			pts=((PostwRect*)gitem)->pair->p;
			prect_pair[0]=pts[0];prect_pair[1]=pts[1];prect_pair[2]=pts[2];prect_pair[3]=pts[3];
		}
		if(gitem->type()==TYPE_FLINE&&gitem->isVisible())
		{
			litem=(FeatureLine*)gitem;
			lns_me.append(QLineF(litem->p1,litem->p2));
			lns_pair.append(QLineF(litem->pair->p1,litem->pair->p2));
		}
	}
}

void ImageFeatureScene::showLines(bool vis)
{
	QList<QGraphicsItem*> gitems=this->items();
	foreach(QGraphicsItem* gitem, gitems)
	{
		if(gitem->type()==TYPE_FLINE)
			gitem->setVisible(vis);
	}
}
void ImageFeatureScene::showPRect(bool vis)
{
	QList<QGraphicsItem*> gitems=this->items();
	foreach(QGraphicsItem* gitem, gitems)
	{
		if(gitem->type()==TYPE_PRECT)
		{
			gitem->setVisible(vis);
			break; // only one
		}
	}
}

void ImageFeatureScene::on_fline_added(FeatureLine* ln)
{
	FeatureLine* newline=new FeatureLine(ln);
	newline->setPair(ln);ln->setPair(newline);
	this->addItem(newline);
}
void ImageFeatureScene::on_fline_removed(FeatureLine* pair_line)
{
	if(pair_line)
	{
		this->removeItem(pair_line);
		delete pair_line;
	}
}

void ImageFeatureScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if(editable&&event->button()==Qt::RightButton)
	{
		dragging=true;
		line_ghost.setP1P2(line_ghost.mapFromScene(event->scenePos()),
			line_ghost.mapFromScene(event->scenePos()));
		line_ghost.setVisible(true);
	}

	if(!editable&&event->button()==Qt::MiddleButton)
		printf("mouse: (%d,%d)\n",(int)event->scenePos().x(),(int)event->scenePos().y());
	
	return QGraphicsScene::mousePressEvent(event);
}
void ImageFeatureScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	if(editable&&dragging)
		line_ghost.setP2(line_ghost.mapFromScene(event->scenePos()));

	return QGraphicsScene::mouseMoveEvent(event);
}
void ImageFeatureScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if(editable&&dragging)
	{
		dragging=false;
		line_ghost.setVisible(false);
		
		if(line_ghost.p1!=line_ghost.p2)
		{
			FeatureLine* newline=new FeatureLine(&line_ghost);
			this->addItem(newline);
			emit fline_added(newline);
		}
	}
	else if(editable&&event->button()==Qt::MiddleButton)
	{
		QGraphicsItem* gitem=itemAt(event->scenePos());
		if(gitem&&gitem->type()==TYPE_FLINE)
		{
			FeatureLine* fpair=((FeatureLine*)gitem)->pair;
			this->removeItem(gitem);
			delete gitem;

			emit fline_removed(fpair);
		}
	}

	return QGraphicsScene::mouseReleaseEvent(event);
}

QGVImageFeature::QGVImageFeature(QWidget *parent)
:QGraphicsView(parent),the_scene(this)
{
	this->setScene(&the_scene);
}
