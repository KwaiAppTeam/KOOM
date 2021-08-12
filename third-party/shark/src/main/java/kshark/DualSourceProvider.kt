package kshark

/**
 * Both a [StreamingSourceProvider] and a [RandomAccessSourceProvider]
 */
interface DualSourceProvider : StreamingSourceProvider, RandomAccessSourceProvider
