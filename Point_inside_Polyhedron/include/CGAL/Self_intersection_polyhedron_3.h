// compute self-intersection of a CGAL triangle polyhedron mesh
// original code from Lutz Kettner

#ifndef CGAL_SELF_INTERSECTION_POLYHEDRON_3
#define CGAL_SELF_INTERSECTION_POLYHEDRON_3

#include <CGAL/box_intersection_d.h>
#include <CGAL/intersections.h>
#include <CGAL/Bbox_3.h>

#include <vector>
#include <exception>

#include <boost/function_output_iterator.hpp>

namespace CGAL {
namespace internal {
template <class Polyhedron, class Kernel, class OutputIterator>
struct Intersect_facets
{
  typedef typename Kernel::Point_3      Point;
  typedef typename Kernel::Vector_3     Vector;
  typedef typename Kernel::Segment_3    Segment;
  typedef typename Kernel::Triangle_3   Triangle;
  typedef typename Polyhedron::Facet_const_handle       Facet_const_handle;
  typedef typename Polyhedron::Facet_const_iterator     Facet_const_iterator;
  typedef typename Polyhedron::Halfedge_const_handle    Halfedge_const_handle;
  typedef typename CGAL::Box_intersection_d::Box_with_handle_d<double, 3, Facet_const_handle> Box;
  mutable OutputIterator m_iterator;

public:

  Intersect_facets(OutputIterator it)
    : m_iterator(it)
  {
  }

  void operator()(const Box* b,
    const Box* c) const
  {
    Halfedge_const_handle h = b->handle()->halfedge();

    // check for shared egde --> no intersection
    if(h->opposite()->facet() == c->handle() ||
      h->next()->opposite()->facet() == c->handle() ||
      h->next()->next()->opposite()->facet() == c->handle())
      return;

    // check for shared vertex --> maybe intersection, maybe not
    Halfedge_const_handle g = c->handle()->halfedge();
    Halfedge_const_handle v;

    if(h->vertex() == g->vertex())
      v = g;
    if(h->vertex() == g->next()->vertex())
      v = g->next();
    if(h->vertex() == g->next()->next()->vertex())
      v = g->next()->next();

    if(v == Halfedge_const_handle())
    {
      h = h->next();
      if(h->vertex() == g->vertex())
  v = g;
      if(h->vertex() == g->next()->vertex())
  v = g->next();
      if(h->vertex() == g->next()->next()->vertex())
  v = g->next()->next();
      if(v == Halfedge_const_handle())
      {
  h = h->next();
  if(h->vertex() == g->vertex())
    v = g;
  if(h->vertex() == g->next()->vertex())
    v = g->next();
  if(h->vertex() == g->next()->next()->vertex())
    v = g->next()->next();
      }
    }

    if(v != Halfedge_const_handle())
    {
      // found shared vertex: 
      CGAL_assertion(h->vertex() == v->vertex());
      // geometric check if the opposite segments intersect the triangles
      Triangle t1( h->vertex()->point(),
  h->next()->vertex()->point(),
  h->next()->next()->vertex()->point());
      Triangle t2( v->vertex()->point(),
  v->next()->vertex()->point(),
  v->next()->next()->vertex()->point());
      Segment s1( h->next()->vertex()->point(),
  h->next()->next()->vertex()->point());
      Segment s2( v->next()->vertex()->point(),
  v->next()->next()->vertex()->point());

      if(CGAL::do_intersect(t1,s2))
      {
  *m_iterator++ = t1;
  *m_iterator++ = t2;
      }
      else
  if(CGAL::do_intersect(t2,s1))
  {
    *m_iterator++ = t1;
    *m_iterator++ = t2;
  }
  return;
    }

    // check for geometric intersection
    Triangle t1( h->vertex()->point(),
      h->next()->vertex()->point(),
      h->next()->next()->vertex()->point());
    Triangle t2( g->vertex()->point(),
      g->next()->vertex()->point(),
      g->next()->next()->vertex()->point());
    if(CGAL::do_intersect(t1, t2))
    {
      *m_iterator++ = t1;
      *m_iterator++ = t2;
    }
  } // end operator ()
}; // end struct Intersect_facets

struct Throw_at_output {
  class Throw_at_output_exception: public std::exception
  { };

  template<class T>
  void operator()(const T& /* t */) const {
    throw Throw_at_output_exception();
  }
};

}// namespace internal

/** 
 * Function computing self intersecting triangles inside a triangulated polyhedron
 * @pre @a polyhedron.is_pure_triangle()
 *
 * @tparam Kernel a %CGAL kernel
 * @tparam Polyhedron a %CGAL polyhedron
 * @tparam OutputIterator iterator holding `Kernel::Triangle_3`
 *
 * @param polyhedron polyhedron on which self intersection test is applied
 * @param out iterator over intersecting facets
 */
template <class Kernel, class Polyhedron, class OutputIterator>
void self_intersect(const Polyhedron& polyhedron, OutputIterator out)
{
  CGAL_assertion(polyhedron.is_pure_triangle());

  typedef typename Polyhedron::Facet_const_iterator     Facet_const_iterator;
  typedef typename Polyhedron::Facet_const_handle       Facet_const_handle;
  typedef typename CGAL::Box_intersection_d::Box_with_handle_d<double, 3, Facet_const_handle> Box;

  // make one box per facet
  std::vector<Box> boxes;
  boxes.reserve(polyhedron.size_of_facets());

  Facet_const_iterator f;
  for(f = polyhedron.facets_begin();
    f != polyhedron.facets_end();
    f++)
    boxes.push_back(Box( f->halfedge()->vertex()->point().bbox() +
    f->halfedge()->next()->vertex()->point().bbox() +
    f->halfedge()->next()->next()->vertex()->point().bbox(),
    f));

  // generate box pointers
  std::vector<const Box*> box_ptr;
  box_ptr.reserve(polyhedron.size_of_facets());
  typename std::vector<Box>::iterator b;
  for(b = boxes.begin();
    b != boxes.end();
    b++)
    box_ptr.push_back(&*b);

  // compute self-intersections filtered out by boxes
  internal::Intersect_facets<Polyhedron,Kernel,OutputIterator> intersect_facets(out);
  std::ptrdiff_t cutoff = 2000;
  CGAL::box_self_intersection_d(box_ptr.begin(), box_ptr.end(),intersect_facets,cutoff);
}

/** 
 * Function computing whether a polyhedron is self-intersecting or not
 * @pre @a polyhedron.is_pure_triangle()
 *
 * @tparam Kernel a %CGAL kernel
 * @tparam Polyhedron a %CGAL polyhedron
 *
 * @param polyhedron polyhedron on which is-self intersecting test is applied
 * @return true if @a polyhedron is self-intersecting
 */
template <class Kernel, class Polyhedron>
bool self_intersect(const Polyhedron& polyhedron)
{
  try 
  {
    typedef boost::function_output_iterator<internal::Throw_at_output> OutputIterator;
    self_intersect<Kernel>(polyhedron, OutputIterator()); 
  }
  catch( internal::Throw_at_output::Throw_at_output_exception& ) 
  { return true; }

  return false;
}

}// namespace CGAL

#endif // CGAL_SELF_INTERSECTION_POLYHEDRON_3
