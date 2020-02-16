AM_CPPFLAGS = \
	$(GLIB_CFLAGS) \
	$(JSON_CFLAGS) \
	-I$(abs_top_srcdir)/include
AM_LDFLAGS = \
	$(GLIB_LIBS)
LDADD = \
	$(abs_top_builddir)/lib/libsatyr.la

check_PROGRAMS = \
	abrt \
	cluster \
	core_thread \
	gdb_sharedlib \
	normalize \
	operating_system \
	rpm \
	strbuf

abrt_SOURCES = abrt.c
cluster_SOURCES = cluster.c
core_thread_SOURCES = core_thread.c
gdb_sharedlib_SOURCES = gdb_sharedlib.c
normalize_SOURCES = normalize.c
operating_system_SOURCES = operating_system.c
rpm_SOURCES = rpm.c
strbuf_SOURCES = strbuf.c

TESTS = $(check_PROGRAMS)

EXTRA_DIST = gdb_stacktraces \
             java_stacktraces \
             ruby_stacktraces \
             js_stacktraces \
             json_files \
             kerneloopses \
             programs \
             python/satyr.py \
             python/core.py \
             python/gdb.py \
             python/java.py \
             python/koops.py \
             python/metrics.py \
             python/misc.py \
             python/python.py \
             python/report.py \
             python/ruby.py \
             python/javascript.py \
             python/test_helpers.py \
             python_stacktraces \
             problem_dir

## ------------ ##
## package.m4.  ##
## ------------ ##

package.m4: Makefile.in
	{ \
             echo '# Signature of the current package.' && \
             echo 'm4_define([AT_PACKAGE_NAME],' && \
             echo '  [@PACKAGE_NAME@])' && \
             echo 'm4_define([AT_PACKAGE_TARNAME],' && \
             echo '  [@PACKAGE_TARNAME@])' && \
             echo 'm4_define([AT_PACKAGE_VERSION],' && \
             echo '  [@PACKAGE_VERSION@])' && \
             echo 'm4_define([AT_PACKAGE_STRING],' && \
             echo '  [@PACKAGE_STRING@])' && \
             echo 'm4_define([AT_PACKAGE_BUGREPORT],' && \
             echo '  [@PACKAGE_BUGREPORT@])'; \
             echo 'm4_define([AT_PACKAGE_URL],' && \
             echo '  [@PACKAGE_URL@])'; \
	} >'package.m4'
EXTRA_DIST += package.m4

## ------------ ##
## Test suite.  ##
## ------------ ##

TESTSUITE_AT =		\
  local.at 		\
  testsuite.at		\
  utils.at 		\
  gdb_frame.at 		\
  gdb_thread.at 	\
  gdb_stacktrace.at  	\
  java_frame.at 	\
  java_thread.at 	\
  java_stacktrace.at 	\
  koops_frame.at	\
  koops_stacktrace.at   \
  core_frame.at         \
  core_stacktrace.at    \
  ruby_frame.at		\
  ruby_stacktrace.at	\
  js_platform.at \
  js_frame.at \
  js_stacktrace.at \
  metrics.at 		\
  report.at		\
  python_bindings.at

EXTRA_DIST += $(TESTSUITE_AT)
TESTSUITE = $(srcdir)/testsuite
EXTRA_DIST += $(TESTSUITE)
MAINTAINERCLEANFILES = Makefile.in $(TESTSUITE)
check_DATA = atconfig atlocal $(TESTSUITE)
DISTCLEANFILES = atconfig
EXTRA_DIST += atlocal.in
EXTRA_DIST += java_testcases.c

TESTSUITEFLAGS =

if !WITH_PYTHON3
TESTSUITEFLAGS += "SKIP_PYTHON3=yes"
endif

atconfig: $(top_builddir)/config.status
	(cd ${top_builddir} && ./config.status ${subdir}/atconfig)

check-local: $(check_DATA)
	$(SHELL) '$(TESTSUITE)' $(TESTSUITEFLAGS)

installcheck-local: $(check_DATA)
	$(SHELL) '$(TESTSUITE)' AUTOTEST_PATH='$(bindir)' $(TESTSUITEFLAGS) ||:

clean-local:
	test ! -f '$(TESTSUITE)' || $(SHELL) '$(TESTSUITE)' --clean

AUTOTEST = $(AUTOM4TE) --language=autotest
$(TESTSUITE): $(TESTSUITE_AT) $(srcdir)/package.m4
	$(AUTOTEST) -I '$(srcdir)' -o $@.tmp $@.at
	mv $@.tmp $@
