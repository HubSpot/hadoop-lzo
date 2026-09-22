package com.hadoop.compression.lzo;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.util.Arrays;
import java.util.Random;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.io.compress.CompressionInputStream;
import org.apache.hadoop.io.compress.CompressionOutputStream;

/**
 * Minimal end-to-end check that the embedded native library loads and can
 * compress and decompress data. It works entirely in memory, so it does not
 * depend on Hadoop's filesystem support (winutils/hadoop.dll) and is suitable
 * as a platform smoke test. Exits non-zero on any failure.
 */
public final class NativeSmokeTest {
  public static void main(String[] args) throws Exception {
    if (!GPLNativeCodeLoader.isNativeCodeLoaded()) {
      throw new IllegalStateException("GPL native code did not load");
    }
    if (!LzoCodec.isNativeLzoLoaded()) {
      throw new IllegalStateException("native lzo did not load");
    }

    LzopCodec codec = new LzopCodec();
    codec.setConf(new Configuration());

    byte[] original = new byte[128 * 1024];
    new Random(42).nextBytes(original);
    // Interleave a repeating pattern so the data actually compresses.
    for (int i = 0; i < original.length; i += 8) {
      original[i] = 'A';
    }

    ByteArrayOutputStream compressedBytes = new ByteArrayOutputStream();
    CompressionOutputStream cos = codec.createOutputStream(compressedBytes);
    cos.write(original);
    cos.finish();
    cos.close();
    byte[] compressed = compressedBytes.toByteArray();

    ByteArrayOutputStream restored = new ByteArrayOutputStream();
    CompressionInputStream cis =
        codec.createInputStream(new ByteArrayInputStream(compressed));
    byte[] buffer = new byte[4096];
    int read;
    while ((read = cis.read(buffer)) != -1) {
      restored.write(buffer, 0, read);
    }
    cis.close();

    if (!Arrays.equals(original, restored.toByteArray())) {
      throw new IllegalStateException("round-trip mismatch");
    }

    System.out.println(
        "SMOKE OK: " + original.length + " bytes round-tripped (compressed to "
            + compressed.length + ")");
  }
}
