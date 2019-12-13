/*
    SapecNG - Next Generation Symbolic Analysis Program for Electric Circuit
    Copyright (C) 2009, Michele Caini

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


//  Two Graphs Common Spanning Trees Algorithm

#ifndef MRT_H
#define MRT_H


#include <boost/config.hpp>

#include <boost/bimap.hpp>
#include <boost/type_traits.hpp>
#include <boost/concept/requires.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/undirected_dfs.hpp>
#include <boost/graph/connected_components.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <vector>
#include <stack>
#include <map>


namespace boost
{


namespace detail {


  template
    <
      class TreeMap,
      class PredMap,
      class DistMap,
      class LowMap,
      class Buffer
    >
  struct bridges_visitor: public default_dfs_visitor
  {
    bridges_visitor(
        TreeMap tree,
        PredMap pred,
        DistMap dist,
        LowMap low,
        Buffer& buffer
      ): tree_(tree), pred_(pred), dist_(dist), low_(low), buffer_(buffer)
    { num_ = -1; }

    template <class Vertex, class Graph>
    void initialize_vertex(const Vertex& u, const Graph& g)
    {
      put(pred_, u, u);
      put(dist_, u, -1);
    }

    template <class Vertex, class Graph>
    void discover_vertex(const Vertex& u, const Graph& g)
    {
      put(dist_, u, ++num_);
      put(low_, u, get(dist_, u));
    }

    template <class Edge, class Graph>
    void tree_edge(const Edge& e, const Graph& g)
    {
      put(pred_, target(e, g), source(e, g));
      put(tree_, target(e, g), e);
    }

    template <class Edge, class Graph>
    void back_edge(const Edge& e, const Graph& g)
    {
      put(low_, source(e, g),
        std::min(get(low_, source(e, g)), get(dist_, target(e, g))));
    }

    template <class Vertex, class Graph>
    void finish_vertex(const Vertex& u, const Graph& g)
    {
      Vertex parent = get(pred_, u);
      if(get(low_, u) > get(dist_, parent))
        buffer_.push(get(tree_, u));
      put(low_, parent,
        std::min(get(low_, parent), get(low_, u)));
    }

    TreeMap tree_;
    PredMap pred_;
    DistMap dist_;
    LowMap low_;
    Buffer& buffer_;
    int num_;
  };


  template <class Buffer>
  struct cycle_finder: public base_visitor< cycle_finder<Buffer> >
  {
    typedef on_back_edge event_filter;
    cycle_finder(): buffer_(0) { }
    cycle_finder(Buffer* buffer)
      : buffer_(buffer) { }
    template <class Edge, class Graph>
    void operator()(const Edge& e, const Graph& g)
      {
        if(buffer_)
          buffer_->push(e);
      }
    Buffer* buffer_;
  };


  template <class DeletedMap>
  struct deleted_edge_status
  {
    deleted_edge_status() { }
    deleted_edge_status(DeletedMap map): map_(map) { }
    template <class Edge>
    bool operator()(const Edge& e) const
      { return (!get(map_, e)); }
    DeletedMap map_;
  };


  template <class InLMap>
  struct inL_edge_status
  {
    inL_edge_status() { }
    inL_edge_status(InLMap map): map_(map) { }
    template <class Edge>
    bool operator()(const Edge& e) const
      { return get(map_, e); }
    InLMap map_;
  };


  template <
    class Graph,
    class Func,
    class Seq,
    class Map
  >
  void rec_mrt_two_graphs_common_spanning_trees
    (
      const Graph& iG,
      bimap<
          bimaps::set_of<int>,
          bimaps::set_of< typename graph_traits<Graph>::edge_descriptor >
        > iG_bimap,
      Map aiG_inL,
      Map diG,
      const Graph& vG,
      bimap<
          bimaps::set_of<int>,
          bimaps::set_of< typename graph_traits<Graph>::edge_descriptor >
        > vG_bimap,
      Map avG_inL,
      Map dvG,
      Func func,
      Seq inL
    )
  {
    typedef graph_traits<Graph> GraphTraits;

    typedef typename GraphTraits::vertex_descriptor vertex_descriptor;
    typedef typename GraphTraits::edge_descriptor edge_descriptor;

    typedef typename Seq::size_type seq_size_type;

    int edges = num_vertices(iG) - 1;
//
//  Using (edges != 0) condition drives to accidental non-tree
//    ((V-1+1)-fake-tree, named here fat-tree) submission.
//  Remove this condition acts as a workaround to the fat-trees problem.
//  Please do not add that condition to improve performance.
//
//  Here previous (wrong) guard is presented:
//     for(seq_size_type i = 0; (i < inL.size()) && (edges != 0); ++i)
//
    {
      for(seq_size_type i = 0; i < inL.size(); ++i)
        if(inL[i])
          --edges;

      if(edges < 0)
        return;
    }

    bool is_tree = (edges == 0);
    if(is_tree) {
      func(inL);
    } else {
      std::map<vertex_descriptor, default_color_type> vertex_color;
      std::map<edge_descriptor, default_color_type> edge_color;

      std::stack<edge_descriptor> iG_buf, vG_buf;
      bool found = false;

      seq_size_type m;
      for(seq_size_type j = 0; j < inL.size() && !found; ++j) {
        if(!inL[j]
            && !get(diG, iG_bimap.left.at(j))
            && !get(dvG, vG_bimap.left.at(j)))
        {
          put(aiG_inL, iG_bimap.left.at(j), true);
          put(avG_inL, vG_bimap.left.at(j), true);

          undirected_dfs(
              make_filtered_graph(iG,
                detail::inL_edge_status< associative_property_map<
                  std::map<edge_descriptor, bool> > >(aiG_inL)),
              make_dfs_visitor(
                detail::cycle_finder< std::stack<edge_descriptor> > (&iG_buf)),
              associative_property_map<
                std::map<vertex_descriptor, default_color_type> >(vertex_color),
              associative_property_map<
                std::map<edge_descriptor, default_color_type> >(edge_color)
            );
          undirected_dfs(
              make_filtered_graph(vG,
                detail::inL_edge_status< associative_property_map<
                  std::map<edge_descriptor, bool> > >(avG_inL)),
              make_dfs_visitor(
                detail::cycle_finder< std::stack<edge_descriptor> > (&vG_buf)),
              associative_property_map<
                std::map<vertex_descriptor, default_color_type> >(vertex_color),
              associative_property_map<
                std::map<edge_descriptor, default_color_type> >(edge_color)
            );

          if(iG_buf.empty() && vG_buf.empty()) {
            inL[j] = true;
            found = true;
            m = j;
          } else {
            while(!iG_buf.empty()) iG_buf.pop();
            while(!vG_buf.empty()) vG_buf.pop();
            put(aiG_inL, iG_bimap.left.at(j), false);
            put(avG_inL, vG_bimap.left.at(j), false);
          }
        }
      }

      if(found) {

        std::stack<edge_descriptor> iG_buf_copy, vG_buf_copy;
        for(seq_size_type j = 0; j < inL.size(); ++j) {
          if(!inL[j]
              && !get(diG, iG_bimap.left.at(j))
              && !get(dvG, vG_bimap.left.at(j)))
          {

            put(aiG_inL, iG_bimap.left.at(j), true);
            put(avG_inL, vG_bimap.left.at(j), true);

            undirected_dfs(
                make_filtered_graph(iG,
                  detail::inL_edge_status< associative_property_map<
                    std::map<edge_descriptor, bool> > >(aiG_inL)),
                make_dfs_visitor(
                  detail::cycle_finder< std::stack<edge_descriptor> > (&iG_buf)),
               associative_property_map<
                  std::map<vertex_descriptor, default_color_type> >(vertex_color),
                associative_property_map<
                  std::map<edge_descriptor, default_color_type> >(edge_color)
              );
            undirected_dfs(
                make_filtered_graph(vG,
                  detail::inL_edge_status< associative_property_map<
                    std::map<edge_descriptor, bool> > >(avG_inL)),
                make_dfs_visitor(
                  detail::cycle_finder< std::stack<edge_descriptor> > (&vG_buf)),
                associative_property_map<
                  std::map<vertex_descriptor, default_color_type> >(vertex_color),
                associative_property_map<
                  std::map<edge_descriptor, default_color_type> >(edge_color)
              );

            if(!iG_buf.empty() || !vG_buf.empty()) {
              while(!iG_buf.empty()) iG_buf.pop();
              while(!vG_buf.empty()) vG_buf.pop();
              put(diG, iG_bimap.left.at(j), true);
              put(dvG, vG_bimap.left.at(j), true);
              iG_buf_copy.push(iG_bimap.left.at(j));
              vG_buf_copy.push(vG_bimap.left.at(j));
            }

            put(aiG_inL, iG_bimap.left.at(j), false);
            put(avG_inL, vG_bimap.left.at(j), false);
          }
        }

        // REC_MRT
        detail::rec_mrt_two_graphs_common_spanning_trees<Graph, Func, Seq, Map>
          (iG, iG_bimap, aiG_inL, diG, vG, vG_bimap, aiG_inL, dvG, func, inL);

        while(!iG_buf_copy.empty()) {
          put(diG, iG_buf_copy.top(), false);
          put(dvG, vG_bimap.left.at(
            iG_bimap.right.at(iG_buf_copy.top())), false);
          iG_buf_copy.pop();
        }
        while(!vG_buf_copy.empty()) {
          put(dvG, vG_buf_copy.top(), false);
          put(diG, iG_bimap.left.at(
            vG_bimap.right.at(vG_buf_copy.top())), false);
          vG_buf_copy.pop();
        }

        inL[m] = false;
        put(aiG_inL, iG_bimap.left.at(m), false);
        put(avG_inL, vG_bimap.left.at(m), false);

        put(diG, iG_bimap.left.at(m), true);
        put(dvG, vG_bimap.left.at(m), true);

        std::map<vertex_descriptor, edge_descriptor> tree_map;
        std::map<vertex_descriptor, vertex_descriptor> pred_map;
        std::map<vertex_descriptor, int> dist_map, low_map;

        detail::bridges_visitor<
            associative_property_map<
                std::map<vertex_descriptor, edge_descriptor>
              >,
            associative_property_map<
                std::map<vertex_descriptor, vertex_descriptor>
              >,
            associative_property_map< std::map<vertex_descriptor, int> >,
            associative_property_map< std::map<vertex_descriptor, int> >,
            std::stack<edge_descriptor>
          >
        iG_vis(
            associative_property_map<
              std::map< vertex_descriptor, edge_descriptor> >(tree_map),
            associative_property_map<
              std::map< vertex_descriptor, vertex_descriptor> >(pred_map),
            associative_property_map<
              std::map< vertex_descriptor, int> >(dist_map),
            associative_property_map<
              std::map< vertex_descriptor, int> >(low_map),
            iG_buf
          ),
        vG_vis(
            associative_property_map<
              std::map< vertex_descriptor, edge_descriptor> >(tree_map),
            associative_property_map<
              std::map< vertex_descriptor, vertex_descriptor> >(pred_map),
            associative_property_map<
              std::map< vertex_descriptor, int> >(dist_map),
            associative_property_map<
              std::map< vertex_descriptor, int> >(low_map),
            vG_buf
          );

        undirected_dfs(make_filtered_graph(iG,
              detail::deleted_edge_status< associative_property_map<
              std::map<edge_descriptor, bool> > >(diG)),
            iG_vis,
            associative_property_map<
              std::map<vertex_descriptor, default_color_type> >(vertex_color),
            associative_property_map<
              std::map<edge_descriptor, default_color_type> >(edge_color)
          );
        undirected_dfs(make_filtered_graph(vG,
              detail::deleted_edge_status< associative_property_map<
              std::map<edge_descriptor, bool> > >(dvG)),
            vG_vis,
            associative_property_map<
              std::map<vertex_descriptor, default_color_type> >(vertex_color),
            associative_property_map<
              std::map<edge_descriptor, default_color_type> >(edge_color)
          );

        found = false;
        std::stack<edge_descriptor> iG_buf_tmp, vG_buf_tmp;
        while(!iG_buf.empty() && !found) {
          if(!inL[iG_bimap.right.at(iG_buf.top())]) {
            put(aiG_inL, iG_buf.top(), true);
            put(avG_inL, vG_bimap.left.at(
              iG_bimap.right.at(iG_buf.top())), true);

            undirected_dfs(
                make_filtered_graph(iG,
                  detail::inL_edge_status< associative_property_map<
                    std::map<edge_descriptor, bool> > >(aiG_inL)),
                make_dfs_visitor(
                  detail::cycle_finder<
                    std::stack<edge_descriptor> > (&iG_buf_tmp)),
                associative_property_map<
                  std::map<
                    vertex_descriptor, default_color_type> >(vertex_color),
                associative_property_map<
                  std::map<edge_descriptor, default_color_type> >(edge_color)
              );
            undirected_dfs(
                make_filtered_graph(vG,
                  detail::inL_edge_status< associative_property_map<
                    std::map<edge_descriptor, bool> > >(avG_inL)),
                make_dfs_visitor(
                  detail::cycle_finder<
                    std::stack<edge_descriptor> > (&vG_buf_tmp)),
                associative_property_map<
                  std::map<
                    vertex_descriptor, default_color_type> >(vertex_color),
                associative_property_map<
                  std::map<edge_descriptor, default_color_type> >(edge_color)
              );

            if(!iG_buf_tmp.empty() || !vG_buf_tmp.empty()) {
              found = true;
            } else {
              while(!iG_buf_tmp.empty()) iG_buf_tmp.pop();
              while(!vG_buf_tmp.empty()) vG_buf_tmp.pop();
              iG_buf_copy.push(iG_buf.top());
            }

            put(aiG_inL, iG_buf.top(), false);
            put(avG_inL, vG_bimap.left.at(
              iG_bimap.right.at(iG_buf.top())), false);
          }
          iG_buf.pop();
        }
        while(!vG_buf.empty() && !found) {
          if(!inL[vG_bimap.right.at(vG_buf.top())]) {
            put(avG_inL, vG_buf.top(), true);
            put(aiG_inL, iG_bimap.left.at(
              vG_bimap.right.at(vG_buf.top())), true);

            undirected_dfs(
                make_filtered_graph(iG,
                  detail::inL_edge_status< associative_property_map<
                    std::map<edge_descriptor, bool> > >(aiG_inL)),
                make_dfs_visitor(
                  detail::cycle_finder<
                    std::stack<edge_descriptor> > (&iG_buf_tmp)),
                associative_property_map<
                  std::map<
                    vertex_descriptor, default_color_type> >(vertex_color),
                associative_property_map<
                  std::map<edge_descriptor, default_color_type> >(edge_color)
              );
            undirected_dfs(
                make_filtered_graph(vG,
                  detail::inL_edge_status< associative_property_map<
                    std::map<edge_descriptor, bool> > >(avG_inL)),
                make_dfs_visitor(
                  detail::cycle_finder<
                    std::stack<edge_descriptor> > (&vG_buf_tmp)),
                associative_property_map<
                  std::map<
                    vertex_descriptor, default_color_type> >(vertex_color),
                associative_property_map<
                  std::map<edge_descriptor, default_color_type> >(edge_color)
              );

            if(!iG_buf_tmp.empty() || !vG_buf_tmp.empty()) {
              found = true;
            } else {
              while(!iG_buf_tmp.empty()) iG_buf_tmp.pop();
              while(!vG_buf_tmp.empty()) vG_buf_tmp.pop();
              vG_buf_copy.push(vG_buf.top());
            }

            put(avG_inL, vG_buf.top(), false);
            put(aiG_inL, iG_bimap.left.at(
              vG_bimap.right.at(vG_buf.top())), false);
          }
          vG_buf.pop();
        }

        if(!found) {

          while(!iG_buf_copy.empty()) {
            inL[iG_bimap.right.at(iG_buf_copy.top())] = true;
            put(aiG_inL, iG_buf_copy.top(), true);
            put(avG_inL, vG_bimap.left.at(
              iG_bimap.right.at(iG_buf_copy.top())), true);
            iG_buf.push(iG_buf_copy.top());
            iG_buf_copy.pop();
          }
          while(!vG_buf_copy.empty()) {
            inL[vG_bimap.right.at(vG_buf_copy.top())] = true;
            put(avG_inL, vG_buf_copy.top(), true);
            put(aiG_inL, iG_bimap.left.at(
              vG_bimap.right.at(vG_buf_copy.top())), true);
            vG_buf.push(vG_buf_copy.top());
            vG_buf_copy.pop();
          }

          // REC_MRT
          detail::rec_mrt_two_graphs_common_spanning_trees<
              Graph, Func, Seq, Map>
            (iG, iG_bimap, aiG_inL, diG, vG, vG_bimap, aiG_inL, dvG, func, inL);

          while(!iG_buf.empty()) {
            inL[iG_bimap.right.at(iG_buf.top())] = false;
            put(aiG_inL, iG_buf.top(), false);
            put(avG_inL, vG_bimap.left.at(
              iG_bimap.right.at(iG_buf.top())), false);
            iG_buf.pop();
          }
          while(!vG_buf.empty()) {
            inL[vG_bimap.right.at(vG_buf.top())] = false;
            put(avG_inL, vG_buf.top(), false);
            put(aiG_inL, iG_bimap.left.at(
              vG_bimap.right.at(vG_buf.top())), false);
            vG_buf.pop();
          }

        }

        put(diG, iG_bimap.left.at(m), false);
        put(dvG, vG_bimap.left.at(m), false);

      }
    }
  }

} // namespace detail



template <class Coll, class Seq>
struct tree_collector
{

public:
  BOOST_CONCEPT_ASSERT((BackInsertionSequence<Coll>));
  BOOST_CONCEPT_ASSERT((RandomAccessContainer<Seq>));
  BOOST_CONCEPT_ASSERT((CopyConstructible<Seq>));

  typedef typename Coll::value_type coll_value_type;
  typedef typename Seq::value_type seq_value_type;

  BOOST_STATIC_ASSERT((is_same<coll_value_type, Seq>::value));
  BOOST_STATIC_ASSERT((is_same<seq_value_type, bool>::value));

  tree_collector(Coll& seqs): seqs_(seqs) { }

  inline void operator()(Seq seq)
    { seqs_.push_back(seq); }

private:
  Coll& seqs_;

};



template <
  class Graph,
  class Order,
  class Func,
  class Seq
>
BOOST_CONCEPT_REQUIRES(
  ((RandomAccessContainer<Order>))
  ((IncidenceGraphConcept<Graph>))
  ((UnaryFunction<Func, void, Seq>))
  ((Mutable_RandomAccessContainer<Seq>))
  ((VertexAndEdgeListGraphConcept<Graph>)),
  (void)
)
mrt_two_graphs_common_spanning_trees
  (
    const Graph& iG,
    Order iG_map,
    const Graph& vG,
    Order vG_map,
    Func func,
    Seq inL
  )
{
  typedef graph_traits<Graph> GraphTraits;

  typedef typename GraphTraits::directed_category directed_category;
  typedef typename GraphTraits::vertex_descriptor vertex_descriptor;
  typedef typename GraphTraits::edge_descriptor edge_descriptor;

  typedef typename GraphTraits::edges_size_type edges_size_type;
  typedef typename GraphTraits::edge_iterator edge_iterator;

  typedef typename Seq::const_iterator seq_const_iterator;
  typedef typename Seq::difference_type seq_diff_type;
  typedef typename Seq::value_type seq_value_type;
  typedef typename Seq::size_type seq_size_type;
  typedef typename Seq::iterator seq_iterator;

  typedef typename Order::const_iterator order_const_iterator;
  typedef typename Order::difference_type order_diff_type;
  typedef typename Order::value_type order_value_type;
  typedef typename Order::size_type order_size_type;
  typedef typename Order::iterator order_iterator;

  BOOST_STATIC_ASSERT((is_same<order_value_type, edge_descriptor>::value));
  BOOST_CONCEPT_ASSERT((Convertible<order_size_type, edges_size_type>));

  BOOST_CONCEPT_ASSERT((Convertible<seq_size_type, edges_size_type>));
  BOOST_STATIC_ASSERT((is_same<seq_value_type, bool>::value));

  BOOST_STATIC_ASSERT((is_same<directed_category, undirected_tag>::value));

  if(num_vertices(iG) != num_vertices(vG))
    return;

  if(inL.size() != num_edges(iG)
      || inL.size() != num_edges(vG))
    return;

  if(iG_map.size() != num_edges(iG)
      || vG_map.size() != num_edges(vG))
    return;

  typedef bimaps::bimap<
      bimaps::set_of< int >,
      bimaps::set_of< order_value_type >
    > bimap_type;
  typedef typename bimap_type::value_type bimap_value;

  bimap_type iG_bimap, vG_bimap;
  for(order_size_type i = 0; i < iG_map.size(); ++i)
    iG_bimap.insert(bimap_value(i, iG_map[i]));
  for(order_size_type i = 0; i < vG_map.size(); ++i)
    vG_bimap.insert(bimap_value(i, vG_map[i]));

  edge_iterator current, last;
  tie(current, last) = edges(iG);
  for(; current != last; ++current)
    if(iG_bimap.right.find(*current) == iG_bimap.right.end())
      return;
  tie(current, last) = edges(vG);
  for(; current != last; ++current)
    if(vG_bimap.right.find(*current) == vG_bimap.right.end())
      return;

  std::stack<edge_descriptor> iG_buf, vG_buf;

  std::map<vertex_descriptor, edge_descriptor> tree_map;
  std::map<vertex_descriptor, vertex_descriptor> pred_map;
  std::map<vertex_descriptor, int> dist_map, low_map;

  detail::bridges_visitor<
      associative_property_map<
          std::map<vertex_descriptor, edge_descriptor>
        >,
      associative_property_map<
          std::map<vertex_descriptor, vertex_descriptor>
        >,
      associative_property_map< std::map<vertex_descriptor, int> >,
      associative_property_map< std::map<vertex_descriptor, int> >,
      std::stack<edge_descriptor>
    >
  iG_vis(
      associative_property_map<
        std::map< vertex_descriptor, edge_descriptor> >(tree_map),
      associative_property_map<
        std::map< vertex_descriptor, vertex_descriptor> >(pred_map),
      associative_property_map<std::map< vertex_descriptor, int> >(dist_map),
      associative_property_map<std::map< vertex_descriptor, int> >(low_map),
      iG_buf
    ),
  vG_vis(
      associative_property_map<
        std::map< vertex_descriptor, edge_descriptor> >(tree_map),
      associative_property_map<
        std::map< vertex_descriptor, vertex_descriptor> >(pred_map),
      associative_property_map<std::map< vertex_descriptor, int> >(dist_map),
      associative_property_map<std::map< vertex_descriptor, int> >(low_map),
      vG_buf
    );

  std::map<vertex_descriptor, default_color_type> vertex_color;
  std::map<edge_descriptor, default_color_type> edge_color;

  undirected_dfs(iG, iG_vis,
      associative_property_map<
        std::map<vertex_descriptor, default_color_type> >(vertex_color),
      associative_property_map<
        std::map<edge_descriptor, default_color_type> >(edge_color)
    );
  undirected_dfs(vG, vG_vis,
      associative_property_map<
        std::map<vertex_descriptor, default_color_type> >(vertex_color),
      associative_property_map<
        std::map<edge_descriptor, default_color_type> >(edge_color)
    );

  while(!iG_buf.empty()) {
    inL[iG_bimap.right.at(iG_buf.top())] = true;
    iG_buf.pop();
  }
  while(!vG_buf.empty()) {
    inL[vG_bimap.right.at(vG_buf.top())] = true;
    vG_buf.pop();
  }

  std::map<edge_descriptor, bool> iG_inL, vG_inL;
  associative_property_map< std::map<edge_descriptor, bool> >
    aiG_inL(iG_inL), avG_inL(vG_inL);

  for(seq_size_type i = 0; i < inL.size(); ++i)
  {
    if(inL[i]) {
      put(aiG_inL, iG_bimap.left.at(i), true);
      put(avG_inL, vG_bimap.left.at(i), true);
    } else {
      put(aiG_inL, iG_bimap.left.at(i), false);
      put(avG_inL, vG_bimap.left.at(i), false);
    }
  }

  undirected_dfs(
      make_filtered_graph(iG,
        detail::inL_edge_status< associative_property_map<
          std::map<edge_descriptor, bool> > >(aiG_inL)),
      make_dfs_visitor(
        detail::cycle_finder< std::stack<edge_descriptor> > (&iG_buf)),
      associative_property_map<
        std::map<vertex_descriptor, default_color_type> >(vertex_color),
      associative_property_map<
        std::map<edge_descriptor, default_color_type> >(edge_color)
    );
  undirected_dfs(
      make_filtered_graph(vG,
        detail::inL_edge_status< associative_property_map<
          std::map<edge_descriptor, bool> > >(avG_inL)),
      make_dfs_visitor(
        detail::cycle_finder< std::stack<edge_descriptor> > (&vG_buf)),
      associative_property_map<
        std::map<vertex_descriptor, default_color_type> >(vertex_color),
      associative_property_map<
        std::map<edge_descriptor, default_color_type> >(edge_color)
    );

  if(iG_buf.empty() && vG_buf.empty()) {

    std::map<edge_descriptor, bool> iG_deleted, vG_deleted;
    associative_property_map< std::map<edge_descriptor, bool> > diG(iG_deleted);
    associative_property_map< std::map<edge_descriptor, bool> > dvG(vG_deleted);

    tie(current, last) = edges(iG);
    for(;current != last; ++current)
      put(diG, *current, false);
    tie(current, last) = edges(vG);
    for(;current != last; ++current)
      put(dvG, *current, false);

    for(seq_size_type j = 0; j < inL.size(); ++j) {
      if(!inL[j]) {
        put(aiG_inL, iG_bimap.left.at(j), true);
        put(avG_inL, vG_bimap.left.at(j), true);

        undirected_dfs(
            make_filtered_graph(iG,
              detail::inL_edge_status< associative_property_map<
                std::map<edge_descriptor, bool> > >(aiG_inL)),
            make_dfs_visitor(
              detail::cycle_finder< std::stack<edge_descriptor> > (&iG_buf)),
            associative_property_map<
              std::map<vertex_descriptor, default_color_type> >(vertex_color),
            associative_property_map<
              std::map<edge_descriptor, default_color_type> >(edge_color)
          );
        undirected_dfs(
            make_filtered_graph(vG,
              detail::inL_edge_status< associative_property_map<
                std::map<edge_descriptor, bool> > >(avG_inL)),
            make_dfs_visitor(
              detail::cycle_finder< std::stack<edge_descriptor> > (&vG_buf)),
            associative_property_map<
              std::map<vertex_descriptor, default_color_type> >(vertex_color),
            associative_property_map<
              std::map<edge_descriptor, default_color_type> >(edge_color)
          );

        if(!iG_buf.empty() || !vG_buf.empty()) {
          while(!iG_buf.empty()) iG_buf.pop();
          while(!vG_buf.empty()) vG_buf.pop();
          put(diG, iG_bimap.left.at(j), true);
          put(dvG, vG_bimap.left.at(j), true);
        }

        put(aiG_inL, iG_bimap.left.at(j), false);
        put(avG_inL, vG_bimap.left.at(j), false);
      }
    }

    int cc = 0;

    std::map<vertex_descriptor, int> com_map;
    cc += connected_components(
        make_filtered_graph(iG,
          detail::deleted_edge_status<associative_property_map<
            std::map<edge_descriptor, bool> > >(diG)),
        associative_property_map<std::map<vertex_descriptor, int> >(com_map)
      );
    cc += connected_components(
        make_filtered_graph(vG,
          detail::deleted_edge_status<associative_property_map<
            std::map<edge_descriptor, bool> > >(dvG)),
        associative_property_map< std::map<vertex_descriptor, int> >(com_map)
      );

    if(cc != 2)
      return;

    // REC_MRT
    detail::rec_mrt_two_graphs_common_spanning_trees<Graph, Func, Seq,
        associative_property_map< std::map<edge_descriptor, bool> > >
      (iG, iG_bimap, aiG_inL, diG, vG, vG_bimap, aiG_inL, dvG, func, inL);

  }

}


template <
  class Graph,
  class Func,
  class Seq
>
BOOST_CONCEPT_REQUIRES(
  ((IncidenceGraphConcept<Graph>))
  ((EdgeListGraphConcept<Graph>)),
  (void)
)
mrt_two_graphs_common_spanning_trees
  (
    const Graph& iG,
    const Graph& vG,
    Func func,
    Seq inL
  )
{
  typedef graph_traits<Graph> GraphTraits;

  typedef typename GraphTraits::edge_descriptor edge_descriptor;
  typedef typename GraphTraits::edges_size_type edges_size_type;
  typedef typename GraphTraits::edge_iterator edge_iterator;

  std::vector<edge_descriptor> iGO, vGO;
  edge_iterator curr, last;

  tie(curr, last) = edges(iG);
  for(; curr != last; ++curr)
    iGO.push_back(*curr);

  tie(curr, last) = edges(vG);
  for(; curr != last; ++curr)
    vGO.push_back(*curr);

  mrt_two_graphs_common_spanning_trees(iG, iGO, vG, vGO, func, inL);
}


} // namespace boost


#endif // MRT_H
