#include "config.h"
#include "Scene_points_with_normal_item.h"
#include <CGAL/Three/Polyhedron_demo_plugin_helper.h>
#include <CGAL/Three/Polyhedron_demo_plugin_interface.h>

#include <CGAL/pca_estimate_normals.h>
#include <CGAL/jet_estimate_normals.h>
#include <CGAL/vcm_estimate_normals.h>
#include <CGAL/mst_orient_normals.h>
#include <CGAL/Timer.h>
#include <CGAL/Memory_sizer.h>

#include <QObject>
#include <QAction>
#include <QMainWindow>
#include <QApplication>
#include <QtPlugin>
#include <QInputDialog>
#include <QMessageBox>

#include "ui_Point_set_normal_estimation_plugin.h"

#if BOOST_VERSION == 105700
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && !defined(BOOST_NO_CXX11_DEFAULTED_FUNCTIONS)
#  define CGAL_DISABLE_NORMAL_ESTIMATION_PLUGIN 1
#endif
#endif

// Concurrency
#ifdef CGAL_LINKED_WITH_TBB
typedef CGAL::Parallel_tag Concurrency_tag;
#else
typedef CGAL::Sequential_tag Concurrency_tag;
#endif
using namespace CGAL::Three;

class Polyhedron_demo_point_set_normal_estimation_plugin :
  public QObject,
  public Polyhedron_demo_plugin_helper
{
  Q_OBJECT
  Q_INTERFACES(CGAL::Three::Polyhedron_demo_plugin_interface)
  Q_PLUGIN_METADATA(IID "com.geometryfactory.PolyhedronDemo.PluginInterface/1.0")

  QAction* actionNormalEstimation;
  QAction* actionNormalInversion;

public:
  void init(QMainWindow* mainWindow, CGAL::Three::Scene_interface* scene_interface, Messages_interface*) {

    scene = scene_interface;
    actionNormalEstimation = new QAction(tr("Point Set Normal Estimation"), mainWindow);
    actionNormalEstimation->setObjectName("actionNormalEstimation");

    actionNormalInversion = new QAction(tr("Point Set Inverse Normal Orientations"), mainWindow);
    actionNormalInversion->setObjectName("actionNormalInversion");
    autoConnectActions();
  }

  QList<QAction*> actions() const {
    return QList<QAction*>() << actionNormalEstimation << actionNormalInversion;
  }

  bool applicable(QAction* action) const {
    Scene_points_with_normal_item* item = qobject_cast<Scene_points_with_normal_item*>(scene->item(scene->mainSelectionIndex()));

    if (action==actionNormalEstimation)
#if CGAL_DISABLE_NORMAL_ESTIMATION_PLUGIN
    return false;
#else
    return item;
#endif
    else
      return item && item->has_normals();
  }

public Q_SLOTS:
  void on_actionNormalEstimation_triggered();
  void on_actionNormalInversion_triggered();

}; // end PS_demo_smoothing_plugin

class Point_set_demo_normal_estimation_dialog : public QDialog, private Ui::NormalEstimationDialog
{
  Q_OBJECT
  public:
    Point_set_demo_normal_estimation_dialog(QWidget* /*parent*/ = 0)
    {
      setupUi(this);
    }

  int pca_neighbors() const { return m_pca_neighbors->value(); }
  int jet_neighbors() const { return m_jet_neighbors->value(); }
  unsigned int convolution_neighbors() const { return m_convolution_neighbors->value(); }
  double convolution_radius() const { return m_convolution_radius->value(); }
  double offset_radius() const { return m_offset_radius->value(); }
  int orient_neighbors() const { return m_orient_neighbors->value(); }

  unsigned int method () const
  {
    if (buttonPCA->isChecked ())       return 0;
    if (buttonJet->isChecked ())       return 1;
    if (buttonVCM->isChecked ())       return 2;
    return -1;
  }
  bool orient () const
  {
    return buttonOrient->isChecked();
  }
  bool use_convolution_radius () const
  {
    return buttonRadius->isChecked();
  }
};


void Polyhedron_demo_point_set_normal_estimation_plugin::on_actionNormalInversion_triggered()
{
  const CGAL::Three::Scene_interface::Item_id index = scene->mainSelectionIndex();

  Scene_points_with_normal_item* item =
    qobject_cast<Scene_points_with_normal_item*>(scene->item(index));

  if(item)
  {
    // Gets point set
    Point_set* points = item->point_set();
    if(points == NULL)
        return;
  
    for(Point_set::iterator it = points->begin(); it != points->end(); ++it){
      it->normal() = -1 * it->normal();
    }
    item->invalidateOpenGLBuffers();
    scene->itemChanged(item);
  }
}

void Polyhedron_demo_point_set_normal_estimation_plugin::on_actionNormalEstimation_triggered()
{
#if !CGAL_DISABLE_NORMAL_ESTIMATION_PLUGIN
  const CGAL::Three::Scene_interface::Item_id index = scene->mainSelectionIndex();

  Scene_points_with_normal_item* item =
    qobject_cast<Scene_points_with_normal_item*>(scene->item(index));

  if(item)
  {
    // Gets point set
    Point_set* points = item->point_set();
    if(points == NULL)
        return;

    // Gets options
    Point_set_demo_normal_estimation_dialog dialog;
    if(!dialog.exec())
      return;
      
    QApplication::setOverrideCursor(Qt::WaitCursor);

    // First point to delete
    Point_set::iterator first_unoriented_point = points->end();

    //***************************************
    // normal estimation
    //***************************************

    if (dialog.method() == 0) // PCA
    {
      CGAL::Timer task_timer; task_timer.start();
      std::cerr << "Estimates normal direction by PCA (k=" << dialog.pca_neighbors() <<")...\n";

      // Estimates normals direction.
      CGAL::pca_estimate_normals<Concurrency_tag>(points->begin(), points->end(),
                                CGAL::make_normal_of_point_with_normal_pmap(Point_set::value_type()),
                                dialog.pca_neighbors());

      std::size_t memory = CGAL::Memory_sizer().virtual_size();
      std::cerr << "Estimates normal direction: " << task_timer.time() << " seconds, "
                                                  << (memory>>20) << " Mb allocated"
                                                  << std::endl;
    }
    else if (dialog.method() == 1) // Jet
    {
      CGAL::Timer task_timer; task_timer.start();
      std::cerr << "Estimates normal direction by Jet Fitting (k=" << dialog.jet_neighbors() <<")...\n";

      // Estimates normals direction.
      CGAL::jet_estimate_normals<Concurrency_tag>(points->begin(), points->end(),
                                CGAL::make_normal_of_point_with_normal_pmap(Point_set::value_type()),
                                dialog.jet_neighbors());

      std::size_t memory = CGAL::Memory_sizer().virtual_size();
      std::cerr << "Estimates normal direction: " << task_timer.time() << " seconds, "
                                                  << (memory>>20) << " Mb allocated"
                                                  << std::endl;
    }
    else if (dialog.method() == 2) // VCM
    {
      CGAL::Timer task_timer; task_timer.start();
      // Estimates normals direction.
      if (dialog.use_convolution_radius())
        {
          std::cerr << "Estimates Normals Direction using VCM (R="
                    << dialog.offset_radius() << " and r=" << dialog.convolution_radius() << ")...\n";

          CGAL::vcm_estimate_normals(points->begin(), points->end(),
                                     CGAL::make_normal_of_point_with_normal_pmap(Point_set::value_type()),
                                     dialog.offset_radius(), dialog.convolution_radius());
        }
      else
        {
          std::cerr << "Estimates Normals Direction using VCM (R="
                    << dialog.offset_radius() << " and k=" << dialog.convolution_neighbors() << ")...\n";

          CGAL::vcm_estimate_normals(points->begin(), points->end(),
                                     CGAL::make_normal_of_point_with_normal_pmap(Point_set::value_type()),
                                     dialog.offset_radius(), dialog.convolution_neighbors());
        }

      std::size_t memory = CGAL::Memory_sizer().virtual_size();
      std::cerr << "Estimates normal direction: " << task_timer.time() << " seconds, "
                                                  << (memory>>20) << " Mb allocated"
                                                  << std::endl;
    }

    item->set_has_normals(true);
    item->setRenderingMode(PointsPlusNormals);

    //***************************************
    // normal orientation
    //***************************************

    if (dialog.orient ())
      {
        CGAL::Timer task_timer; task_timer.start();
        std::cerr << "Orient normals with a Minimum Spanning Tree (k=" << dialog.orient_neighbors() << ")...\n";

        // Tries to orient normals
        first_unoriented_point =
          CGAL::mst_orient_normals(points->begin(), points->end(),
                                   CGAL::make_normal_of_point_with_normal_pmap(Point_set::value_type()),
                                   dialog.orient_neighbors());

        //indicates that the point set has normals
        if (first_unoriented_point!=points->begin()){
          item->set_has_normals(true);
          item->setRenderingMode(PointsPlusNormals);
        }

        std::size_t nb_unoriented_normals = std::distance(first_unoriented_point, points->end());
        std::size_t memory = CGAL::Memory_sizer().virtual_size();
        std::cerr << "Orient normals: " << nb_unoriented_normals << " point(s) with an unoriented normal are selected ("
                  << task_timer.time() << " seconds, "
                  << (memory>>20) << " Mb allocated)"
                  << std::endl;

        // Selects points with an unoriented normal
        points->set_first_selected (first_unoriented_point);

        // Updates scene
        item->invalidateOpenGLBuffers();
        scene->itemChanged(index);

        QApplication::restoreOverrideCursor();

        // Warns user
        if (nb_unoriented_normals > 0)
          {
            QMessageBox::information(NULL,
                                     tr("Points with an unoriented normal"),
                                     tr("%1 point(s) with an unoriented normal are selected.\nPlease orient them or remove them before running Poisson reconstruction.")
                                     .arg(nb_unoriented_normals));
          }
      }
    else
      {
        // Updates scene
        item->invalidateOpenGLBuffers();
        scene->itemChanged(index);

        QApplication::restoreOverrideCursor();
      }
  }
#endif // !CGAL_DISABLE_NORMAL_ESTIMATION_PLUGIN
}

#include "Point_set_normal_estimation_plugin.moc"
