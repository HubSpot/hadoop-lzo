Hadoop-LZO (HubSpot fork)
==========

Hadoop-LZO is a project to bring splittable LZO compression to Hadoop.  LZO is an ideal compression format for Hadoop due to its combination of speed and compression size.  However, LZO files are not natively splittable, meaning the parallelism that is the core of Hadoop is gone.  This project re-enables that parallelism with LZO compressed files, and also comes with standard utilities (input/output streams, etc) for working with LZO files.

### Origins

This project builds off the great work done at [https://code.google.com/p/hadoop-gpl-compression](https://code.google.com/p/hadoop-gpl-compression).

### Hadoop and LZO, Together at Last

LZO is a wonderful compression scheme to use with Hadoop because it's incredibly fast, and (with a bit of work) it's splittable.  Gzip is decently fast, but cannot take advantage of Hadoop's natural map splits because it's impossible to start decompressing a gzip stream starting at a random offset in the file.  LZO's block format makes it possible to start decompressing at certain specific offsets of the file -- those that start new LZO block boundaries.  In addition to providing LZO decompression support, these classes provide an in-process indexer (com.hadoop.compression.lzo.LzoIndexer) and a map-reduce style indexer which will read a set of LZO files and output the offsets of LZO block boundaries that occur near the natural Hadoop block boundaries.  This enables a large LZO file to be split into multiple mappers and processed in parallel.  Because it is compressed, less data is read off disk, minimizing the number of IOPS required.  And LZO decompression is so fast that the CPU stays ahead of the disk read, so there is no performance impact from having to decompress data as it's read off disk.

You can read more about Hadoop, LZO, and how we're using it at Twitter at [https://www.cloudera.com/blog/2009/11/17/hadoop-at-twitter-part-1-splittable-lzo-compression/](https://www.cloudera.com/blog/2009/11/17/hadoop-at-twitter-part-1-splittable-lzo-compression/).

### Usage

We publish JARs that contain native code builds for the following platforms:
- Linux x86_64
- Linux aarch64
- MacOS x86_64
- MacOS aarch64
- Windows x86_64

The entire LZO library, plus Java bindings, are provided in the JARs, so the only system dependency is libc.

Unlike other forks of hadoop-lzo, **you can use our pre-built JARs on any of our supported platforms and it should Just Work.**

### Versioning

Version numbers are based on the LZO library version, with this wrapper library's version appended. For example, `2.10-1` is the first wrapper version around LZO 2.10.
