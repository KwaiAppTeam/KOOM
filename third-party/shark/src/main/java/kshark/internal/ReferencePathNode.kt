package kshark.internal

import kshark.GcRoot
import kshark.LeakTraceReference
import kshark.LibraryLeakReferenceMatcher

sealed class ReferencePathNode {
  abstract val objectId: Long

  interface LibraryLeakNode {
    val matcher: LibraryLeakReferenceMatcher
  }

  sealed class RootNode : ReferencePathNode() {
    abstract val gcRoot: GcRoot

    class LibraryLeakRootNode(
        override val objectId: Long,
        override val gcRoot: GcRoot,
        override val matcher: LibraryLeakReferenceMatcher
    ) : RootNode(), LibraryLeakNode

    class NormalRootNode(
        override val objectId: Long,
        override val gcRoot: GcRoot
    ) : RootNode()

  }

  sealed class ChildNode : ReferencePathNode() {

    abstract val parent: ReferencePathNode

    /**
     * The reference from the parent to this node
     */
    abstract val refFromParentType: LeakTraceReference.ReferenceType
    abstract val refFromParentName: String

    //在祖类的字段引用了此Node
    abstract val declaredClassName: String

    class LibraryLeakChildNode(
        override val objectId: Long,
        override val parent: ReferencePathNode,
        override val refFromParentType: LeakTraceReference.ReferenceType,
        override val refFromParentName: String,
        override val matcher: LibraryLeakReferenceMatcher,
        override val declaredClassName: String
    ) : ChildNode(), LibraryLeakNode

    class NormalNode(
        override val objectId: Long,
        override val parent: ReferencePathNode,
        override val refFromParentType: LeakTraceReference.ReferenceType,
        override val refFromParentName: String,
        override val declaredClassName: String
    ) : ChildNode()
  }

}