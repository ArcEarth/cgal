
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>

#include <CGAL/jet_estimate_normals.h>

#include <vector>

#include <iostream>


typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;

// Simple geometric types
typedef Kernel::FT FT;
typedef Kernel::Point_3 Point_3;
typedef Kernel::Vector_3 Vector_3;




int main()
{  
  std::vector<Point_3> points;
  std::vector<size_t> indices(100);

  for(int i=0; i < 100; i++){
    indices[i] = i;
  }
  std::vector<Vector_3> normals(100);


  for(int i=0; i <10; i++){
    for(int j=0; j <10; j++){
      points.push_back(Point_3(i,j,0));
    }
  }


  CGAL::jet_estimate_normals<CGAL::Sequential_tag>(indices.begin(), indices.end(),
                                                   &(points[0]),
                                                   &(normals[0]),
                                                   12);
  
  
  return 0;
}

