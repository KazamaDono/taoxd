/**
 * @name Network-tainted memcpy length
 * @description The length argument of memcpy is reachable from a
 *              read/recv/recvfrom without an intervening bound.
 * @kind path-problem
 * @problem.severity error
 * @id cpp/network-tainted-memcpy
 */
import cpp
import semmle.code.cpp.dataflow.new.TaintTracking
import NetTaintFlow::PathGraph

module NetTaintConfig implements DataFlow::ConfigSig {
  predicate isSource(DataFlow::Node src) {                        // ❶
    exists(FunctionCall c |
      c.getTarget().hasName(["read", "recv", "recvfrom"]) and
      src.asExpr() = c.getArgument(1)
    )
  }

  predicate isSink(DataFlow::Node sink) {                         // ❷
    exists(FunctionCall c |
      c.getTarget().hasName("memcpy") and
      sink.asExpr() = c.getArgument(2)
    )
  }

  predicate isBarrier(DataFlow::Node node) {                      // ❸
    exists(GuardCondition g, Expr bound |
      g.controls(node.asExpr().getBasicBlock(), _) and
      g.getAChild*() = node.asExpr() and
      g.getAChild*() = bound and
      bound.getType().getUnspecifiedType() instanceof IntegralType
    )
  }
}

module NetTaintFlow = TaintTracking::Global<NetTaintConfig>;

from NetTaintFlow::PathNode source, NetTaintFlow::PathNode sink
where NetTaintFlow::flowPath(source, sink)
select sink.getNode(), source, sink,
  "memcpy length is tainted from $@.", source.getNode(), "network read"
