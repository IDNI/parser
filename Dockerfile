# This is a Dockerfile for building, testing, packaging and running
# the Tau Parser Library and TGF tool

# To build the image with a tag 'tgf' from ./Dockerfile:
#   docker build -t tgf .

# use --build-arg TESTS="no" to skip running tests
# use --build-arg BUILD_TYPE="Debug" for building of the debugging version

# To run tgf using the created image in interactive mode 
# with a tgf file mounted to /grammar.tgf:
#   docker run --rm -it -v <tgf file>:/grammar.tgf tgf /grammar.tgf [<tgf options>]

# --rm flag is used to remove the container after it exits


FROM ubuntu:24.04

# Install dependencies
RUN apt-get update && apt-get install -y \
	bash wget git nsis rpm doxygen graphviz \
	cmake=3.28.3-1build7 \
	g++=4:13.2.0-7ubuntu1 \
	mingw-w64=11.0.1-3build1 \
	python3-distutils-extra
        
# Argument BUILD_TYPE=Debug/Release
ARG BUILD_TYPE=Release

# Argument RELEASE=yes is used to build release packages
ARG RELEASE=no

# Argument NIGHTLY=yes is used to build nightly packages (works only if RELEASE=yes)
ARG NIGHTLY=no

# Argument TESTS=no is used to skip running tests
ARG TESTS=yes

# Argument DOCUMENTATION=no skips building of API documentation
ARG DOCUMENTATION=yes

# Copy source code
COPY ./ /parser

# # OR clone from git
# RUN git clone https://github.com/IDNI/parser /parser

WORKDIR /parser

RUN ./dev clean all

# if NIGHTLY is set to yes, then add .YYYY-MM-DD to the first line of the VERSION file
RUN if [ "$NIGHTLY" = "yes" ]; then \
	echo -n "$(head -n 1 VERSION)-$(date --iso)" > VERSION; \
fi
RUN echo "(BUILD) -- Building version: $(head -n 1 VERSION)"

# Build tests and run them if TESTS is set to yes. Stop the build if they fail
RUN echo " (BUILD) -- Running tests: $TESTS"
RUN if [ "$TESTS" = "yes" ]; then \
	./dev build "${BUILD_TYPE}" -DTAU_PARSER_BUILD_TESTS=ON && \
	cd tests && \
	ctest -j 8 --test-dir "../build-${BUILD_TYPE}" --output-on-failure \
		|| exit 1; \
fi

RUN echo "(BUILD) -- Building packages: $RELEASE (nightly: $NIGHTLY)"

# Documentation
RUN if [ "$DOCUMENTATION" = "yes" ] || [ "$RELEASE" = "yes" ]; then \
	./dev build "${BUILD_TYPE}" -DTAU_PARSER_BUILD_DOC=ON; \
fi

# Windows packages
RUN if [ "$RELEASE" = "yes" ]; then \
	./dev w64-packages \
	&& rm ./build-Release/CMakeCache.txt; \
fi

# Linux packages
RUN if [ "$RELEASE" = "yes" ]; then \
	./dev packages; \
fi

# If tgf executable does not exist already, build it
RUN if [ ! -f ./build-${BUILD_TYPE}/tgf ]; then \
	./dev build "${BUILD_TYPE}" -DTAU_PARSER_BUILD_TGF=ON; \
fi

# Set the entrypoint to the tgf executable
WORKDIR /parser/build-${BUILD_TYPE}
ENTRYPOINT [ "./tgf" ]
CMD []
