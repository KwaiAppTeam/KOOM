package kshark

import okio.Source

/**
 * Can open [RandomAccessSource] instances.
 */
interface RandomAccessSourceProvider {
  fun openRandomAccessSource(): RandomAccessSource
}
