package juga999;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

import java.util.Arrays;

import static org.junit.jupiter.api.Assertions.assertArrayEquals;

class MatMulVulkanTests {

    @Test
    void testMultiplications() {
        String libDir = System.getProperty("matmul.dir");
        if (libDir == null || libDir.isBlank()) {
            throw new IllegalArgumentException(
                    "Missing required parameter: -Dmatmul.dir=/path/to/libraryDir"
            );
        }

        MatMulVulkan.Context context = MatMulVulkan.Context.create(libDir);

        {
            int[] matrix1 = new int[]{1, 2, 3, 4};
            int[] matrix2 = new int[]{5, 6, 7, 8};
            int[] result = new int[4];
            Arrays.fill(result, 0);
            try (MatMulVulkan.Session session = context.newSession(2, 2, 2)) {
                session.compute(matrix1, matrix2, result);
                assertArrayEquals(new int[]{19, 22, 43, 50}, result);
            }
        }
        {
            int dim = 1024;
            int[] matrix1 = new int[dim * dim];
            int[] matrix2 = new int[dim * dim];
            int[] result = new int[dim * dim];
            for (int i = 0; i < dim; ++i) {
                matrix1[i * dim + i] = 1;
            }
            for (int i = 0; i < matrix2.length; ++i) {
                matrix2[i] = i % 32 + 1;
            }
            Arrays.fill(result, 0);
            try (MatMulVulkan.Session session = context.newSession(dim, dim, dim)) {
                session.compute(matrix1, matrix2, result);
                Assertions.assertEquals(matrix2[0], result[0]);
                Assertions.assertEquals(matrix2[256], result[256]);
                Assertions.assertEquals(matrix2[512], result[512]);
            }
        }
    }

}
