/*
 * This file is part of Hootenanny.
 *
 * Hootenanny is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * --------------------------------------------------------------------
 *
 * The following copyright notices are generated automatically. If you
 * have a new notice to add, please use the format:
 * " * @copyright Copyright ..."
 * This will properly maintain the copyright information. DigitalGlobe
 * copyrights will be updated automatically.
 *
 * @copyright Copyright (C) 2016, 2017, 2018, 2019 DigitalGlobe (http://www.digitalglobe.com/)
 */
#ifndef CONFLICTSNETWORKMATCHER_H
#define CONFLICTSNETWORKMATCHER_H

// hoot
#include <hoot/core/elements/OsmMap.h>
#include <hoot/core/algorithms/optimizer/SingleAssignmentProblemSolver.h>
#include <hoot/core/conflate/network/IndexedEdgeMatchSet.h>
#include <hoot/core/conflate/network/NetworkMatcher.h>
#include <hoot/core/conflate/network/NetworkEdgeScore.h>
#include <hoot/core/conflate/network/NetworkVertexScore.h>
#include <hoot/core/conflate/network/OsmNetwork.h>

// tgs
#include <tgs/RStarTree/HilbertRTree.h>
#include <tgs/RStarTree/IntersectionIterator.h>

namespace hoot
{

class ConflictsNetworkMatcherTest;

/**
 *
 */
class ConflictsNetworkMatcher : public NetworkMatcher
{
public:

  static std::string className() { return "hoot::ConflictsNetworkMatcher"; }

  const static double EPSILON;

  /**
   * Always construct with create() to make a shared pointer.
   */
  ConflictsNetworkMatcher();

  /**
   * Use this instead of a constructor.
   */
  static boost::shared_ptr<ConflictsNetworkMatcher> create();

  // move to config - #2913
  virtual double getMatchThreshold() const { return 0.35; }

  virtual void iterate();

  virtual void matchNetworks(ConstOsmMapPtr map, OsmNetworkPtr n1, OsmNetworkPtr n2);

  virtual void finalize();

  virtual QList<NetworkEdgeScorePtr> getAllEdgeScores() const;

  virtual QList<NetworkVertexScorePtr> getAllVertexScores() const;

  virtual IndexedEdgeMatchSetPtr getMatches() { return _edgeMatches; }

private:

  // for white box testing.
  friend class ConflictsNetworkMatcherTest;

  typedef SingleAssignmentProblemSolver<EdgeString, EdgeString> Saps;

  typedef QHash<ConstEdgeMatchPtr, double> EdgeScoreMap;

  class MatchRelationship
  {
  public:

    MatchRelationship(ConstEdgeMatchPtr e, bool conflict) : _e(e), _conflict(conflict) {}

    ConstEdgeMatchPtr getEdge() const { return _e; }

    /**
     * Returns a set of all matches that connect this edge to e. Typically this will be zero or
     * one, but more than one is technically possible.
     */
    QSet<ConstEdgeMatchPtr> getThroughStubs() const { return _throughStub; }

    bool isConflict() const { return _conflict; }

    void setThroughStubs(QSet<ConstEdgeMatchPtr> stubs) { _throughStub = stubs; }

  private:

    ConstEdgeMatchPtr _e;
    bool _conflict;
    QSet<ConstEdgeMatchPtr> _throughStub;
  };
  typedef boost::shared_ptr<const MatchRelationship> ConstMatchRelationshipPtr;
  typedef boost::shared_ptr<MatchRelationship> MatchRelationshipPtr;

  typedef QHash<ConstEdgeMatchPtr, QList<ConstMatchRelationshipPtr>> MatchRelationshipMap;

  IndexedEdgeMatchSetPtr _edgeMatches;
  EdgeScoreMap _scores, _weights;

  /// A larger value will increase the weight of partial matches
  double _partialHandicap;
  /// A larger value will increase the weight of stubs
  double _stubHandicap;
  /// A larger value will conflate more aggressively (fewer reviews)
  double _aggression;
  double _stubThroughWeighting;
  /// A value of 0 will cause all edges to have the same weight with each neighbor, a higher value
  /// will give matches with more support a higher weight.
  double _weightInfluence;

  /// A value of 0 will cause an edge to contribute (1 * score * weight) to each neighbor. A value
  /// of 1 will give approx (1 / n * score * weight) influence to each neighbor.
  double _outboundWeighting;

  MatchRelationshipMap _matchRelationships;

  double _aggregateScores(QList<double> pairs);

  void _createEmptyStubEdges(OsmNetworkPtr na, OsmNetworkPtr nb);

  /* Gets the separation between the pairs in the match, in terms of Frechet distance */
  Meters _getMatchSeparation(ConstEdgeMatchPtr pMatch);

  /* Remove matches that don't make sense, based on frechet distance */
  void _sanityCheckRelationships();

  void _createMatchRelationships();

  QList<ConstNetworkEdgePtr> _getEdgesOnVertex(ConstNetworkVertexPtr v);

  QSet<ConstEdgeMatchPtr> _getMatchesWithSharedTermination(ConstNetworkVertexPtr v1,
    ConstNetworkVertexPtr v2) const;

  void _iterateSimple();

  void _iterateRank();

  void _seedEdgeScores();

  void _printEdgeMatches();
};

typedef boost::shared_ptr<ConflictsNetworkMatcher> ConflictsNetworkMatcherPtr;
typedef boost::shared_ptr<const ConflictsNetworkMatcher> ConstConflictsNetworkMatcherPtr;

// not implemented
bool operator<(ConstConflictsNetworkMatcherPtr, ConstConflictsNetworkMatcherPtr);

}

#endif // CONFLICTSNETWORKMATCHER_H
