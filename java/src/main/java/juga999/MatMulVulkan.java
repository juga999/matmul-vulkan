package juga999;

import java.lang.foreign.Arena;
import java.lang.foreign.FunctionDescriptor;
import java.lang.foreign.Linker;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.SymbolLookup;
import java.lang.invoke.MethodHandle;
import java.nio.file.Path;

import static java.lang.foreign.ValueLayout.ADDRESS;
import static java.lang.foreign.ValueLayout.JAVA_INT;

public final class MatMulVulkan {

    public static final String LIB_NAME = "libmatmul_vulkan.so";
    public static final String SHADER_NAME = "matmul.spv";

    public static final class Context {
        private final String libDir;
        private MethodHandle matmulContextDestroyMethod;
        private MethodHandle matmulComputeMethod;
        private MethodHandle matmulContextCreateMethod;

        public static Context create(String libDir) {
            Context context = new Context(libDir);
            SymbolLookup lib = SymbolLookup.libraryLookup(Path.of(libDir, LIB_NAME), Arena.global());
            MemorySegment matmulContextCreateSymbol = lib.find("matmul_context_create").orElseThrow();
            MemorySegment matmulComputeSymbol = lib.find("matmul_compute").orElseThrow();
            MemorySegment matmulContextDestroySymbol = lib.find("matmul_context_destroy").orElseThrow();

            Linker linker = Linker.nativeLinker();
            context.matmulContextDestroyMethod = linker.downcallHandle(matmulContextDestroySymbol,
                    FunctionDescriptor.ofVoid(ADDRESS));

            context.matmulComputeMethod = linker.downcallHandle(matmulComputeSymbol,
                    FunctionDescriptor.ofVoid(ADDRESS, ADDRESS, ADDRESS, ADDRESS));

            context.matmulContextCreateMethod = linker.downcallHandle(matmulContextCreateSymbol,
                    FunctionDescriptor.of(ADDRESS, ADDRESS, JAVA_INT, JAVA_INT, JAVA_INT));

            return context;
        }

        public Session newSession(int m, int k, int n) {
            return new Session(this, m, k, n);
        }

        public String getLibDir() {
            return libDir;
        }

        public MethodHandle getMatmulContextDestroyMethod() {
            return matmulContextDestroyMethod;
        }

        public MethodHandle getMatmulComputeMethod() {
            return matmulComputeMethod;
        }

        public MethodHandle getMatmulContextCreateMethod() {
            return matmulContextCreateMethod;
        }

        private Context(String libDir) {
            this.libDir = libDir;
        }
    }

    public static final class Session implements AutoCloseable {

        private final Context context;
        private MemorySegment handle;

        Session(Context context, int m, int k, int n) {
            this.context = context;
            try (Arena arena = Arena.ofConfined()) {

                MemorySegment shaderPath = arena.allocateFrom(Path.of(context.getLibDir(), SHADER_NAME).toString());

                handle = (MemorySegment) context.getMatmulContextCreateMethod().invokeExact(shaderPath, m, k, n);
            } catch (Throwable e) {
                throw new RuntimeException(e);
            }
        }

        void compute(int[] matrixA, int[] matrixB, int[] result) {
            try (Arena arena = Arena.ofConfined()) {
                MemorySegment nativeMatrixA = copy(arena, matrixA);
                MemorySegment nativeMatrixB = copy(arena, matrixB);

                MemorySegment nativeMatrixC =  arena.allocate(JAVA_INT, result.length);

                context.getMatmulComputeMethod().invokeExact(handle,
                        nativeMatrixA, nativeMatrixB, nativeMatrixC);
                copy(nativeMatrixC, result);
            } catch (Throwable e) {
                throw new RuntimeException(e);
            }
        }

        @Override
        public void close() {
            if (handle == null) {
                return;
            }

            try {
                context.getMatmulContextDestroyMethod().invokeExact(handle);
            } catch (Throwable e) {
                throw new RuntimeException("Destruction failed", e);
            } finally {
                handle = null;
            }
        }

        private static MemorySegment copy(Arena arena, int[] src) {
            long size  = (long) src.length * Integer.BYTES;
            MemorySegment newSegment = arena.allocate(size);
            MemorySegment.copy(
                    MemorySegment.ofArray(src), 0,
                    newSegment,0,
                    size
            );
            return newSegment;
        }

        private static void copy(MemorySegment src, int[] dest) {
            long size  = (long) dest.length * Integer.BYTES;
            MemorySegment.copy(
                    src, 0,
                    MemorySegment.ofArray(dest),0,
                    size
            );
        }
    }

    private MatMulVulkan() {}
}
