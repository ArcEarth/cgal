#include <CGAL/Mesh_3/io_signature.h>
#include "Scene_c3t3_item.h"

#include <CGAL/Three/Polyhedron_demo_io_plugin_interface.h>
#include <CGAL/Three/Polyhedron_demo_plugin_interface.h>
#include <CGAL/IO/File_avizo.h>
#include <iostream>
#include <fstream>


class Polyhedron_demo_c3t3_binary_io_plugin :
  public QObject,
  public CGAL::Three::Polyhedron_demo_io_plugin_interface,
  public CGAL::Three::Polyhedron_demo_plugin_interface
{
    Q_OBJECT
    Q_INTERFACES(CGAL::Three::Polyhedron_demo_io_plugin_interface)
    Q_INTERFACES(CGAL::Three::Polyhedron_demo_plugin_interface)
    Q_PLUGIN_METADATA(IID "com.geometryfactory.PolyhedronDemo.PluginInterface/1.0")

public:
  void init(QMainWindow*, CGAL::Three::Scene_interface* sc, Messages_interface*)
  {
    this->scene = sc;
  }
  QString name() const { return "C3t3_io_plugin"; }
  QString nameFilters() const { return "binary files (*.cgal);;ascii (*.mesh);;maya (*.ma)"; }
  QString saveNameFilters() const { return "binary files (*.cgal);;ascii (*.mesh);;maya (*.ma);;avizo (*.am)"; }
  QString loadNameFilters() const { return "binary files (*.cgal)" ; }
  QList<QAction*> actions() const
  {
    return QList<QAction*>();
  }
  bool applicable(QAction*) const
  {
    return false;
  }
  bool canLoad() const;
  CGAL::Three::Scene_item* load(QFileInfo fileinfo);

  bool canSave(const CGAL::Three::Scene_item*);
  bool save(const CGAL::Three::Scene_item*, QFileInfo fileinfo);

private:
  bool try_load_other_binary_format(std::istream& in, C3t3& c3t3);
  bool try_load_a_cdt_3(std::istream& in, C3t3& c3t3);
  CGAL::Three::Scene_interface* scene;
};


bool Polyhedron_demo_c3t3_binary_io_plugin::canLoad() const {
  return true;
}


CGAL::Three::Scene_item*
Polyhedron_demo_c3t3_binary_io_plugin::load(QFileInfo fileinfo) {


    if(fileinfo.suffix().toLower() == "cgal")
    {
        // Open file
        std::ifstream in(fileinfo.filePath().toUtf8(),
                         std::ios_base::in|std::ios_base::binary);
        if(!in) {
          std::cerr << "Error! Cannot open file "
                    << (const char*)fileinfo.filePath().toUtf8() << std::endl;
          return NULL;
        }

        Scene_c3t3_item* item = new Scene_c3t3_item();
        item->setName(fileinfo.baseName());
        item->setScene(scene);


        if(item->load_binary(in)) {
          return item;
        }

        item->c3t3().clear();
        in.seekg(0);
        if(try_load_other_binary_format(in, item->c3t3())) {
          item->c3t3_changed();
          item->changed();
          return item;
        }

        item->c3t3().clear();
        in.seekg(0);
        if(try_load_a_cdt_3(in, item->c3t3())) {
          item->c3t3_changed();
          item->changed();
          return item;
        }
    }



  // if all loading failed...
  return NULL;
}

bool Polyhedron_demo_c3t3_binary_io_plugin::canSave(const CGAL::Three::Scene_item* item)
{
  // This plugin supports c3t3 items.
  return qobject_cast<const Scene_c3t3_item*>(item);
}

bool
Polyhedron_demo_c3t3_binary_io_plugin::
save(const CGAL::Three::Scene_item* item, QFileInfo fileinfo)
{
    const Scene_c3t3_item* c3t3_item = qobject_cast<const Scene_c3t3_item*>(item);
    if ( NULL == c3t3_item )
    {
      return false;
    }

    QString path = fileinfo.absoluteFilePath();

    if(path.endsWith(".cgal"))
    {
    std::ofstream out(fileinfo.filePath().toUtf8(),
                     std::ios_base::out|std::ios_base::binary);

    return out && c3t3_item->save_binary(out);
    }

   else  if (fileinfo.suffix() == "mesh")
    {
      std::ofstream medit_file (qPrintable(path));
      c3t3_item->c3t3().output_to_medit(medit_file,true,true);
          return true;
    }
    else if (fileinfo.suffix() == "ma")
    {
      std::ofstream maya_file (qPrintable(path));
      c3t3_item->c3t3().output_to_maya(
        maya_file,true);
          return true;
    }
    else if (fileinfo.suffix() == "am")
    {
      std::ofstream avizo_file (qPrintable(path));
      CGAL::output_to_avizo(avizo_file, c3t3_item->c3t3());
      return true;
    }
    else
        return false;
}

struct Fake_mesh_domain {
  typedef CGAL::Tag_true Has_features;
  typedef int Subdomain_index;
  typedef std::pair<int,int> Surface_patch_index;
  typedef int Curve_segment_index;
  typedef int Corner_index;
  typedef boost::variant<Subdomain_index,Surface_patch_index> Index;
};

typedef Geom_traits Fake_gt;
typedef CGAL::Mesh_vertex_base_3<Fake_gt, Fake_mesh_domain> Fake_vertex_base;
typedef CGAL::Compact_mesh_cell_base_3<Fake_gt, Fake_mesh_domain> Fake_cell_base;
typedef CGAL::Triangulation_data_structure_3<Fake_vertex_base,Fake_cell_base> Fake_tds;
typedef CGAL::Regular_triangulation_3<Fake_gt, Fake_tds> Fake_tr;
typedef CGAL::Mesh_complex_3_in_triangulation_3<
  Fake_tr,
  Fake_mesh_domain::Corner_index,
  Fake_mesh_domain::Curve_segment_index> Fake_c3t3;

template <class Vb = CGAL::Triangulation_vertex_base_3<Kernel> >
struct Fake_CDT_3_vertex_base : public Vb
{
  typedef Vb Base;
  bool steiner;
  std::size_t ref_1, ref_2;

  template < typename TDS2 >
  struct Rebind_TDS {
    typedef typename Base::template Rebind_TDS<TDS2>::Other   Vb2;
    typedef Fake_CDT_3_vertex_base<Vb2>  Other;
  };
};

template <class Vb>
std::istream&
operator>>( std::istream& is, Fake_CDT_3_vertex_base<Vb>& v)
{
  is >> static_cast<typename Fake_CDT_3_vertex_base<Vb>::Base&>(v);
  char s;
  if( CGAL::is_ascii(is) ) {
    is >> s;
    if( s == 'S' ) {
      v.steiner = true;
      is >> v.ref_1 >> v.ref_2;
    }
    else {
      CGAL_assertion(s == '.' || s == 'F');
      v.steiner = false;
    }
  } else {
    CGAL::read( is, s );
    if(is.bad()) return is;
    if( s == 'S' ) {
      v.steiner = true;
      CGAL::read( is, v.ref_1 );
      CGAL::read( is, v.ref_2 );
    }
    else {
      // if(s != '.') {
      // 	std::cerr << "v.point()=" << v.point() << std::endl;
      // 	std::cerr << "s=" << s << " (" << (int)s
      // 		  << "), just before position "
      // 		  << is.tellg() << " !\n";
      // }
      CGAL_assertion(s == '.' || s== 'F');
      v.steiner = false;
    }
  }
  return is;
}

template <class Cb = CGAL::Triangulation_cell_base_3<Kernel> >
struct Fake_CDT_3_cell_base : public Cb
{
  typedef Cb Base;
  int constrained_facet[4];
  bool _restoring[6];
  int to_edge_index( int li, int lj ) const {
    CGAL_triangulation_precondition( li >= 0 && li < 4 );
    CGAL_triangulation_precondition( lj >= 0 && lj < 4 );
    CGAL_triangulation_precondition( li != lj );
    return ( li==0 || lj==0 ) ? li+lj-1 : li+lj;
  }

  template < typename TDS2 >
  struct Rebind_TDS {
    typedef typename Base::template Rebind_TDS<TDS2>::Other   Cb2;
    typedef Fake_CDT_3_cell_base<Cb2>  Other;
  };
};

template <typename Cb>
std::istream&
operator>>( std::istream& is, Fake_CDT_3_cell_base<Cb>& c) {
  char s;
  for( int li = 0; li < 4; ++li ) {
    if( CGAL::is_ascii(is) )
      is >> c.constrained_facet[li];
    else
      CGAL::read( is, c.constrained_facet[li] );
  }

  if( CGAL::is_ascii(is) ) {
    is >> s;
    CGAL_assertion(s == '-');
  }
  is >> static_cast<typename Fake_CDT_3_cell_base<Cb>::Base&>(c);
  for( int li = 0; li < 3; ++li ) {
    for( int lj = li+1; lj < 4; ++lj ) {
      char s;
      is >> s;
      if(s == 'C') {
        c._restoring[c.to_edge_index(li, lj)] = true;
      } else {
        if(s != '.') {
          std::cerr << "cDT cell:";
          for( int li = 0; li < 4; ++li ) {
            std::cerr << " " << c.constrained_facet[li];
          }
          std::cerr << "\n";
          std::cerr << "s=" << s << " (" << (int)s
                    << "), just before position "
                    << is.tellg() << " !\n";	}
        CGAL_assertion(s == '.');
        c._restoring[c.to_edge_index(li, lj)] = false;
      }
    }
  }
  return is;
}

typedef CGAL::Triangulation_data_structure_3<Fake_CDT_3_vertex_base<>, Fake_CDT_3_cell_base<> > Fake_CDT_3_TDS;
typedef CGAL::Triangulation_3<Kernel, Fake_CDT_3_TDS> Fake_CDT_3;

typedef Fake_mesh_domain::Surface_patch_index Fake_patch_id;

#include <CGAL/Mesh_3/io_signature.h>

#ifdef CGAL_MESH_3_IO_SIGNATURE_H
namespace CGAL {
template <>
struct Get_io_signature<Fake_patch_id> {
  std::string operator()() const
  {
    return std::string("std::pair<i,i>");
  }
}; // end Get_io_signature<Fake_patch_id>
} // end namespace CGAL
#endif

namespace std {
  inline std::ostream& operator<<(std::ostream& out, const Fake_patch_id& id) {
    return out << id.first << " " << id.second;
  }
  inline std::istream& operator>>(std::istream& in, Fake_patch_id& id) {
    return in >> id.first >> id.second;
  }
} // end namespace std

namespace CGAL {
template <>
class Output_rep<Fake_patch_id> {
  typedef Fake_patch_id T;
  const T& t;
public:
  //! initialize with a const reference to \a t.
  Output_rep( const T& tt) : t(tt) {}
  //! perform the output, calls \c operator\<\< by default.
  std::ostream& operator()( std::ostream& out) const {
    if(is_ascii(out)) {
      out << t.first << " " << t.second;
    } else {
      CGAL::write(out, t.first);
      CGAL::write(out, t.second);
    }
    return out;
  }
};

template <>
class Input_rep<Fake_patch_id> {
  typedef Fake_patch_id T;
  T& t;
public:
  //! initialize with a const reference to \a t.
  Input_rep( T& tt) : t(tt) {}
  //! perform the output, calls \c operator\<\< by default.
  std::istream& operator()( std::istream& in) const {
    if(is_ascii(in)) {
      in >> t.first >> t.second;
    } else {
      CGAL::read(in, t.first);
      CGAL::read(in, t.second);
    }
    return in;
  }
};
} // end namespace CGAL

struct Update_vertex {
  typedef Fake_mesh_domain::Surface_patch_index Sp_index;
  template <typename V1, typename V2>
  bool operator()(const V1& v1, V2& v2) {
    v2.set_point(v1.point());
    v2.set_dimension(v1.in_dimension());
    v2.set_special(v1.is_special());
    switch(v1.in_dimension()) {
    case 0:
    case 1:
    case 3:
      v2.set_index(boost::get<int>(v1.index()));
      break;
    default: // case 2
      const typename V1::Index& index = v1.index();
      const Sp_index sp_index = boost::get<Sp_index>(index);
      v2.set_index((std::max)(sp_index.first, sp_index.second));
    }
    return true;
  }
}; // end struct Update_vertex

struct Update_cell {
  typedef Fake_mesh_domain::Surface_patch_index Sp_index;
  template <typename C1, typename C2>
  bool operator()(const C1& c1, C2& c2) {
    c2.set_subdomain_index(c1.subdomain_index());
    for(int i = 0; i < 4; ++i) {
      const Sp_index sp_index = c1.surface_patch_index(i);
      c2.set_surface_patch_index(i, (std::max)(sp_index.first,
                                               sp_index.second));
      CGAL_assertion(c1.is_facet_on_surface(i) ==
                     c2.is_facet_on_surface(i));
    }
    return true;
  }
}; // end struct Update_cell

#include <CGAL/Triangulation_file_input.h>

struct Update_vertex_from_CDT_3 {
  template <typename V1, typename V2>
  bool operator()(const V1& v1, V2& v2) {
    v2.set_point(v1.point());
    v2.set_dimension(2);
    v2.set_special(false);
    return true;
  }
}; // end struct Update_vertex

struct Update_cell_from_CDT_3 {
  typedef Fake_mesh_domain::Surface_patch_index Sp_index;
  template <typename C1, typename C2>
  bool operator()(const C1& c1, C2& c2) {
    c2.set_subdomain_index(1);
    for(int i = 0; i < 4; ++i) {
      c2.set_surface_patch_index(i, c1.constrained_facet[i]);
    }
    return true;
  }
}; // end struct Update_cell

bool
Polyhedron_demo_c3t3_binary_io_plugin::
try_load_a_cdt_3(std::istream& is, C3t3& c3t3)
{
  std::cerr << "Try load a CDT_3...";
  CGAL::set_binary_mode(is);
  if(CGAL::file_input<
     Fake_CDT_3,
     C3t3::Triangulation,
     Update_vertex_from_CDT_3,
     Update_cell_from_CDT_3>(is, c3t3.triangulation()))
  {
    c3t3.rescan_after_load_of_triangulation();
    std::cerr << "Try load a CDT_3... DONE";
    return true;
  }
  else {
    return false;
  }
}
//Generates a compilation error.
bool
Polyhedron_demo_c3t3_binary_io_plugin::
try_load_other_binary_format(std::istream& is, C3t3& c3t3)
{
  CGAL::set_ascii_mode(is);
  std::string s;
  if(!(is >> s)) return false;
  bool binary = false;
  if(s == "binary") {
    binary = true;
    if(!(is >> s)) return false;
  }
  if( s != "CGAL" ||
      !(is >> s) ||
      s != "c3t3")
  {
    return false;
  }
  std::getline(is, s);
  if(s != "") {
    if(s != std::string(" ") + CGAL::Get_io_signature<Fake_c3t3>()()) {
      std::cerr << "Polyhedron_demo_c3t3_binary_io_plugin::try_load_other_binary_format:"
                << "\n  expected format: " << CGAL::Get_io_signature<Fake_c3t3>()()
                << "\n       got format:" << s << std::endl;
      return false;
    }
  }
  if(binary) CGAL::set_binary_mode(is);
  else CGAL::set_ascii_mode(is);
  std::istream& f_is = CGAL::file_input<
    Fake_c3t3::Triangulation,
    C3t3::Triangulation,
    Update_vertex,
    Update_cell>(is, c3t3.triangulation());

  c3t3.rescan_after_load_of_triangulation();
  return f_is.good();
}

#include <QtPlugin>
#include "C3t3_io_plugin.moc"
