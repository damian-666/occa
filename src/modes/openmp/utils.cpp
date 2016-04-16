/* The MIT License (MIT)
 *
 * Copyright (c) 2014 David Medina and Tim Warburton
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 */

#if OCCA_OPENMP_ENABLED


namespace occa {
  namespace openmp {
    std::string notSupported = "N/A";

    std::string baseCompilerFlag(const int vendor_){
      if(vendor_ & (sys::vendor::GNU |
                    sys::vendor::LLVM)){

        return "-fopenmp";
      }
      else if(vendor_ & (sys::vendor::Intel |
                         sys::vendor::Pathscale)){

        return "-openmp";
      }
      else if(vendor_ & sys::vendor::IBM){
        return "-qsmp";
      }
      else if(vendor_ & sys::vendor::PGI){
        return "-mp";
      }
      else if(vendor_ & sys::vendor::HP){
        return "+Oopenmp";
      }
      else if(vendor_ & sys::vendor::VisualStudio){
        return "/openmp";
      }
      else if(vendor_ & sys::vendor::Cray){
        return "";
      }

      return omp::notSupported;
    }

    std::string compilerFlag(const int vendor_,
                             const std::string &compiler){

#if (OCCA_OS & (LINUX_OS | OSX_OS))
      std::stringstream ss;
      std::string flag = omp::notSupported;

      const std::string safeCompiler = removeSlashes(compiler);
      const std::string &hash = safeCompiler;

      const std::string testFilename   = sys::filename("[occa]/testing/ompTest.cpp");
      const std::string binaryFilename = sys::filename("[occa]/testing/omp_" + safeCompiler);
      const std::string infoFilename   = sys::filename("[occa]/testing/ompInfo_" + safeCompiler);

      cacheFile(testFilename,
                io::read(env::OCCA_DIR + "/scripts/ompTest.cpp"),
                "ompTest");

      if(!haveHash(hash)){
        waitForHash(hash);
      } else {
        if(!sys::fileExists(infoFilename)){
          flag = baseCompilerFlag(vendor_);
          ss << compiler
             << ' '
             << flag
             << ' '
             << testFilename
             << " -o "
             << binaryFilename
             << " > /dev/null 2>&1";

          const int compileError = system(ss.str().c_str());

          if(compileError)
            flag = omp::notSupported;

          io::write(infoFilename, flag);
          releaseHash(hash);

          return flag;
        }
        releaseHash(hash);
      }

      ss << io::read(infoFilename);
      ss >> flag;

      return flag;
#elif (OCCA_OS == WINDOWS_OS)
      return "/openmp"; // VS Compilers support OpenMP
#endif
    }
  }
}

#endif
