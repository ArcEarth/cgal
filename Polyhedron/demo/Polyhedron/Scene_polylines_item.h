#ifndef SCENE_POLYLINES_ITEM_H
#define SCENE_POLYLINES_ITEM_H
#include "Scene_polylines_item_config.h"
#include <CGAL/Three/Viewer_interface.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Three/Scene_group_item.h>

#include <QString>
#include <QMenu>

#include <list>
#include <vector>

class Scene_polylines_item_private;
class Scene_spheres_item;

class SCENE_POLYLINES_ITEM_EXPORT Scene_polylines_item : public CGAL::Three::Scene_group_item
{
    Q_OBJECT
public:
    typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
    typedef K::Point_3 Point_3;
    typedef std::vector<Point_3> Polyline;
    typedef std::list<Polyline> Polylines_container;

    Scene_polylines_item();
    virtual ~Scene_polylines_item();

    bool isFinite() const { return true; }
    bool isEmpty() const;
    void compute_bbox() const;

    Scene_polylines_item* clone() const;

    QString toolTip() const;

    // Indicate if rendering mode is supported
    bool supportsRenderingMode(RenderingMode m) const;

    QMenu* contextMenu();

    // Flat/Gouraud OpenGL drawing
    void draw() const {}
    void draw(CGAL::Three::Viewer_interface*) const;

    // Wireframe OpenGL drawing
    void drawEdges() const{}
    void drawEdges(CGAL::Three::Viewer_interface*) const;

    void drawPoints() const{}
    void drawPoints(CGAL::Three::Viewer_interface*) const;


    void smooth(std::vector<Point_3>& polyline){
        bool is_closed = polyline.front()==polyline.back();
        typedef K::Vector_3 Vector_3;

        std::size_t start = is_closed ? 0:1;
        std::size_t end   = polyline.size()-1;

        Vector_3 prev = (is_closed ? polyline[end-1] : polyline[0]) - CGAL::ORIGIN;

        for (std::size_t i=start; i!=end; ++i)
        {
            Vector_3 curr = polyline[i] - CGAL::ORIGIN;
            Vector_3 next = polyline[i+1] - CGAL::ORIGIN;

            polyline[i] = CGAL::ORIGIN+(prev+2*curr+next)/4;
            prev=curr;
        }

        if (is_closed) polyline[end]=polyline[0];
    }

public Q_SLOTS:
    virtual void invalidateOpenGLBuffers();
    void change_corner_radii(double);
    void change_corner_radii();
    void split_at_sharp_angles();
    void reset_spheres()
    {
      spheres = NULL;
    }

    void merge(Scene_polylines_item*);

    void smooth(){
        for (Polylines_container::iterator pit=polylines.begin(),pit_end=polylines.end();pit!=pit_end;++pit)
            smooth(*pit);
      invalidateOpenGLBuffers();
      Q_EMIT itemChanged();
    }
public:
    Polylines_container polylines;

    // http://en.wikipedia.org/wiki/D-pointer
    Scene_polylines_item_private* d;
private:

    enum VAOs {
        Edges=0,
        NbOfVaos
    };
    enum VBOs {
        Edges_Vertices = 0,
        NbOfVbos
    };

    mutable Scene_spheres_item *spheres;
    mutable std::vector<float> positions_lines;
    mutable std::size_t nb_lines;
    typedef std::map<Point_3, int> Point_to_int_map;
    typedef Point_to_int_map::iterator iterator;
    void computeSpheres();
    using CGAL::Three::Scene_item::initializeBuffers;
    void initializeBuffers(CGAL::Three::Viewer_interface *viewer) const;
    using CGAL::Three::Scene_item::computeElements;
    void computeElements() const;



}; // end class Scene_polylines_item

#endif
