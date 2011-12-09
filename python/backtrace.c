#include "common.h"
#include "strbuf.h"

static PyMethodDef BacktraceMethods[] = {
    /* methods */
    { "dup",                  p_btp_backtrace_dup,                  METH_NOARGS,  b_dup_doc                  },
    { "find_crash_frame",     p_btp_backtrace_find_crash_frame,     METH_NOARGS,  b_find_crash_frame_doc     },
    { "find_crash_thread",    p_btp_backtrace_find_crash_thread,    METH_NOARGS,  b_find_crash_thread_doc    },
    { "limit_frame_depth",    p_btp_backtrace_limit_frame_depth,    METH_VARARGS, b_limit_frame_depth_doc    },
    { "quality_simple",       p_btp_backtrace_quality_simple,       METH_NOARGS,  b_quality_simple_doc       },
    { "quality_complex",      p_btp_backtrace_quality_complex,      METH_NOARGS,  b_quality_complex_doc      },
    { "get_duplication_hash", p_btp_backtrace_get_duplication_hash, METH_NOARGS,  b_get_duplication_hash_doc },
    { NULL },
};

static PyMemberDef BacktraceMembers[] = {
    { (char *)"threads",     T_OBJECT_EX, offsetof(BacktraceObject, threads),     0,        b_threads_doc     },
    { (char *)"crashframe",  T_OBJECT_EX, offsetof(BacktraceObject, crashframe),  READONLY, b_crashframe_doc  },
    { (char *)"crashthread", T_OBJECT_EX, offsetof(BacktraceObject, crashthread), READONLY, b_crashthread_doc },
    { NULL },
};

PyTypeObject BacktraceTypeObject = {
    PyObject_HEAD_INIT(NULL)
    0,
    "btparser.Backtrace",           /* tp_name */
    sizeof(BacktraceObject),        /* tp_basicsize */
    0,                              /* tp_itemsize */
    p_btp_backtrace_free,           /* tp_dealloc */
    NULL,                           /* tp_print */
    NULL,                           /* tp_getattr */
    NULL,                           /* tp_setattr */
    NULL,                           /* tp_compare */
    NULL,                           /* tp_repr */
    NULL,                           /* tp_as_number */
    NULL,                           /* tp_as_sequence */
    NULL,                           /* tp_as_mapping */
    NULL,                           /* tp_hash */
    NULL,                           /* tp_call */
    p_btp_backtrace_str,            /* tp_str */
    NULL,                           /* tp_getattro */
    NULL,                           /* tp_setattro */
    NULL,                           /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,             /* tp_flags */
    backtrace_doc,                  /* tp_doc */
    NULL,                           /* tp_traverse */
    NULL,                           /* tp_clear */
    NULL,                           /* tp_richcompare */
    0,                              /* tp_weaklistoffset */
    NULL,                           /* tp_iter */
    NULL,                           /* tp_iternext */
    BacktraceMethods,               /* tp_methods */
    BacktraceMembers,               /* tp_members */
    NULL,                           /* tp_getset */
    NULL,                           /* tp_base */
    NULL,                           /* tp_dict */
    NULL,                           /* tp_descr_get */
    NULL,                           /* tp_descr_set */
    0,                              /* tp_dictoffset */
    NULL,                           /* tp_init */
    NULL,                           /* tp_alloc */
    p_btp_backtrace_new,            /* tp_new */
    NULL,                           /* tp_free */
    NULL,                           /* tp_is_gc */
    NULL,                           /* tp_bases */
    NULL,                           /* tp_mro */
    NULL,                           /* tp_cache */
    NULL,                           /* tp_subclasses */
    NULL,                           /* tp_weaklist */
};

/* helpers */
int backtrace_prepare_linked_list(BacktraceObject *backtrace)
{
    int i;
    PyObject *item;
    ThreadObject *current = NULL, *prev = NULL;
    for (i = 0; i < PyList_Size(backtrace->threads); ++i)
    {
        item = PyList_GetItem(backtrace->threads, i);
        if (!item)
            return -1;

        Py_INCREF(item);
        if (!PyObject_TypeCheck(item, &ThreadTypeObject))
        {
            Py_XDECREF(current);
            Py_XDECREF(prev);
            PyErr_SetString(PyExc_TypeError, "threads must be a list of btparser.Thread objects");
            return -1;
        }

        current = (ThreadObject *)item;
        if (thread_prepare_linked_list(current) < 0)
            return -1;

        if (i == 0)
            backtrace->backtrace->threads = current->thread;
        else
            prev->thread->next = current->thread;

        Py_XDECREF(prev);
        prev = current;
    }

    current->thread->next = NULL;
    Py_XDECREF(current);

    return 0;
}

PyObject *backtrace_prepare_thread_list(struct btp_backtrace *backtrace)
{
    PyObject *result = PyList_New(0);
    if (!result)
        return PyErr_NoMemory();

    Py_INCREF(result);

    struct btp_thread *thread = backtrace->threads;
    ThreadObject *item;
    while (thread)
    {
        item = (ThreadObject *)p_btp_thread_new(&ThreadTypeObject, PyTuple_New(0), NULL);
        Py_INCREF(item);
        btp_thread_free(item->thread);
        item->thread = thread;

        Py_CLEAR(item->frames);
        item->frames = thread_prepare_frame_list(thread);
        if (!item->frames)
            return NULL;

        if (PyList_Append(result, (PyObject *)item) < 0)
            return NULL;

        thread = thread->next;
    }

    return result;
}

/* constructor */
PyObject *p_btp_backtrace_new(PyTypeObject *object, PyObject *args, PyObject *kwds)
{
    BacktraceObject *bo = (BacktraceObject *)PyObject_MALLOC(sizeof(BacktraceObject));
    if (!bo)
        return PyErr_NoMemory();

    const char *str = NULL;
    if (!PyArg_ParseTuple(args, "|s", &str))
        return NULL;

    PyObject_INIT(bo, &BacktraceTypeObject);
    bo->crashframe = (FrameObject *)Py_None;
    bo->crashthread = (ThreadObject *)Py_None;
    if (str)
    {
        /* ToDo parse */
        struct btp_location location;
        btp_location_init(&location);
        bo->backtrace = btp_backtrace_parse(&str, &location);
        if (!bo->backtrace)
        {
            PyErr_SetString(PyExc_ValueError, location.message);
            return NULL;
        }
        bo->threads = backtrace_prepare_thread_list(bo->backtrace);
        if (!bo->threads)
            return NULL;
    }
    else
    {
        bo->threads = PyList_New(0);
        bo->backtrace = btp_backtrace_new();
    }

    return (PyObject *)bo;
}

/* destructor */
void p_btp_backtrace_free(PyObject *object)
{
    BacktraceObject *this = (BacktraceObject *)object;
    this->backtrace->threads = NULL;
    btp_backtrace_free(this->backtrace);
    PyObject_Del(object);
}

/* str */
PyObject *p_btp_backtrace_str(PyObject *self)
{
    BacktraceObject *this = (BacktraceObject *)self;
    struct btp_strbuf *buf = btp_strbuf_new();
    btp_strbuf_append_strf(buf, "Backtrace with %d threads",
                           PyList_Size(this->threads));
    char *str = btp_strbuf_free_nobuf(buf);
    PyObject *result = Py_BuildValue("s", str);
    free(str);
    return result;
}

/* methods */
PyObject *p_btp_backtrace_dup(PyObject *self, PyObject *args)
{
    BacktraceObject *this = (BacktraceObject *)self;
    if (backtrace_prepare_linked_list(this) < 0)
        return NULL;

    BacktraceObject *bo = (BacktraceObject *)PyObject_MALLOC(sizeof(BacktraceObject));
    if (!bo)
        return PyErr_NoMemory();
    PyObject_INIT(bo, &BacktraceTypeObject);

    bo->backtrace = btp_backtrace_dup(this->backtrace);
    if (!bo->backtrace)
        return NULL;

    bo->threads = backtrace_prepare_thread_list(bo->backtrace);
    if (!bo->threads)
        return NULL;

    if (PyObject_TypeCheck(this->crashthread, &ThreadTypeObject))
    {
        bo->crashthread = (ThreadObject *)p_btp_thread_dup((PyObject *)this->crashthread, PyTuple_New(0));
        if (!bo->crashthread)
            return NULL;
    }
    else
        bo->crashthread = (ThreadObject *)Py_None;

    if (PyObject_TypeCheck(this->crashframe, &FrameTypeObject))
    {
        bo->crashframe = (FrameObject *)p_btp_thread_dup((PyObject *)this->crashframe, PyTuple_New(0));
        if (!bo->crashframe)
            return NULL;
    }
    else
        bo->crashframe = (FrameObject *)Py_None;

    return (PyObject *)bo;
}

PyObject *p_btp_backtrace_find_crash_frame(PyObject *self, PyObject *args)
{
    BacktraceObject *this = (BacktraceObject *)self;
    if (backtrace_prepare_linked_list(this) < 0)
        return NULL;

    FrameObject *result = (FrameObject *)p_btp_frame_new(&FrameTypeObject, PyTuple_New(0), NULL);
    if (!result)
        return PyErr_NoMemory();

    btp_frame_free(result->frame);
    struct btp_backtrace *tmp = btp_backtrace_dup(this->backtrace);
    result->frame = btp_backtrace_get_crash_frame(tmp);
    if (!result->frame)
    {
        btp_backtrace_free(tmp);
        PyErr_SetString(PyExc_LookupError, "Crash frame not found");
        return NULL;
    }
    this->crashframe = result;
    btp_backtrace_free(tmp);

    return (PyObject *)result;
}

PyObject *p_btp_backtrace_find_crash_thread(PyObject *self, PyObject *args)
{
    BacktraceObject *this = (BacktraceObject *)self;
    if (backtrace_prepare_linked_list(this) < 0)
        return NULL;

    struct btp_thread *thread = btp_backtrace_find_crash_thread(this->backtrace);
    if (!thread)
    {
        PyErr_SetString(PyExc_LookupError, "Crash thread not found");
        return NULL;
    }
    ThreadObject *result = (ThreadObject *)p_btp_thread_new(&ThreadTypeObject, PyTuple_New(0), NULL);
    if (!result)
        return PyErr_NoMemory();

    Py_CLEAR(result->frames);
    result->frames = thread_prepare_frame_list(thread);
    if (!result->frames)
        return NULL;

    this->crashthread = result;
    Py_CLEAR(this->threads);
    this->threads = backtrace_prepare_thread_list(this->backtrace);
    if (!this->threads)
        return NULL;

    return (PyObject *)result;
}

PyObject *p_btp_backtrace_limit_frame_depth(PyObject *self, PyObject *args)
{
    BacktraceObject *this = (BacktraceObject *)self;
    if (backtrace_prepare_linked_list(this) < 0)
        return NULL;

    int depth;
    if (!PyArg_ParseTuple(args, "i", &depth))
        return NULL;

    btp_backtrace_limit_frame_depth(this->backtrace, depth);
    Py_CLEAR(this->threads);
    this->threads = backtrace_prepare_thread_list(this->backtrace);
    if (!this->threads)
        return NULL;

    Py_RETURN_NONE;
}

PyObject *p_btp_backtrace_quality_simple(PyObject *self, PyObject *args)
{
    BacktraceObject *this = (BacktraceObject *)self;
    if (backtrace_prepare_linked_list(this) < 0)
        return NULL;

    float result = btp_backtrace_quality_simple(this->backtrace);
    Py_CLEAR(this->threads);
    this->threads = backtrace_prepare_thread_list(this->backtrace);
    if (!this->threads)
        return NULL;

    return Py_BuildValue("f", result);
}

PyObject *p_btp_backtrace_quality_complex(PyObject *self, PyObject *args)
{
    BacktraceObject *this = (BacktraceObject *)self;
    if (backtrace_prepare_linked_list(this) < 0)
        return NULL;

    float result = btp_backtrace_quality_complex(this->backtrace);
    Py_CLEAR(this->threads);
    this->threads = backtrace_prepare_thread_list(this->backtrace);
    if (!this->threads)
        return NULL;

    return Py_BuildValue("f", result);
}

PyObject *p_btp_backtrace_get_duplication_hash(PyObject *self, PyObject *args)
{
    BacktraceObject *this = (BacktraceObject *)self;
    if (backtrace_prepare_linked_list(this) < 0)
        return NULL;

    char *duphash = btp_backtrace_get_duplication_hash(this->backtrace);
    PyObject *result = Py_BuildValue("s", duphash);
    free(duphash);
    Py_CLEAR(this->threads);
    this->threads = backtrace_prepare_thread_list(this->backtrace);
    if (!this->threads)
        return NULL;

    return result;
}
