/* Copyright 2015 the SumatraPDF project authors (see AUTHORS file).
   License: Simplified BSD (see COPYING.BSD) */

// include BaseUtil.h instead of including directly

// auto-free memory for arbitrary malloc()ed memory of type T*
template <typename T>
class ScopedMem {
  public:
    T* ptr;

    ScopedMem() : ptr(nullptr) {}
    explicit ScopedMem(T* ptr) : ptr(ptr) {}
    ~ScopedMem() { free(ptr); }
    void Set(T* newPtr) {
        free(ptr);
        ptr = newPtr;
    }
    T* Get() const { return ptr; }
    T* StealData() {
        T* tmp = ptr;
        ptr = nullptr;
        return tmp;
    }
    operator T*() const { return ptr; }
};

// deletes any object at the end of the scope
template <class T>
class ScopedPtr {
    T* obj;

  public:
    ScopedPtr() : obj(nullptr) {}
    explicit ScopedPtr(T* obj) : obj(obj) {}
    ~ScopedPtr() { delete obj; }
    T* Detach() {
        T* tmp = obj;
        obj = nullptr;
        return tmp;
    }
    operator T*() const { return obj; }
    T* operator->() const { return obj; }
    T* operator=(T* newObj) {
        delete obj;
        return (obj = newObj);
    }
};

template <typename T>
class AutoFreeStr : public ScopedMem<T> {
  public:
    AutoFreeStr() { this->ptr = nullptr; }
    explicit AutoFreeStr(T* ptr) { this->ptr = ptr; }
    void SetCopy(const T* newPtr) {
        free(this->ptr);
        this->ptr = nullptr;
        if (newPtr) {
            this->ptr = str::Dup(newPtr);
        }
    }
};

typedef AutoFreeStr<char> AutoFree;

#if OS(WIN)
typedef AutoFreeStr<WCHAR> AutoFreeW;

class ScopedCritSec {
    CRITICAL_SECTION* cs;

  public:
    explicit ScopedCritSec(CRITICAL_SECTION* cs) : cs(cs) { EnterCriticalSection(cs); }
    ~ScopedCritSec() { LeaveCriticalSection(cs); }
};

class ScopedHandle {
    HANDLE handle;

  public:
    explicit ScopedHandle(HANDLE handle) : handle(handle) {}
    ~ScopedHandle() { CloseHandle(handle); }
    operator HANDLE() const { return handle; }
    bool IsValid() const { return handle != NULL && handle != INVALID_HANDLE_VALUE; }
};

template <class T>
class ScopedComPtr {
  protected:
    T* ptr;

  public:
    ScopedComPtr() : ptr(nullptr) {}
    explicit ScopedComPtr(T* ptr) : ptr(ptr) {}
    ~ScopedComPtr() {
        if (ptr)
            ptr->Release();
    }
    bool Create(const CLSID clsid) {
        CrashIf(ptr);
        if (ptr)
            return false;
        HRESULT hr = CoCreateInstance(clsid, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&ptr));
        return SUCCEEDED(hr);
    }
    operator T*() const { return ptr; }
    T** operator&() { return &ptr; }
    T* operator->() const { return ptr; }
    T* operator=(T* newPtr) {
        if (ptr)
            ptr->Release();
        return (ptr = newPtr);
    }
};

template <class T>
class ScopedComQIPtr {
  protected:
    T* ptr;

  public:
    ScopedComQIPtr() : ptr(nullptr) {}
    explicit ScopedComQIPtr(IUnknown* unk) {
        HRESULT hr = unk->QueryInterface(&ptr);
        if (FAILED(hr))
            ptr = nullptr;
    }
    ~ScopedComQIPtr() {
        if (ptr)
            ptr->Release();
    }
    bool Create(const CLSID clsid) {
        CrashIf(ptr);
        if (ptr)
            return false;
        HRESULT hr = CoCreateInstance(clsid, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&ptr));
        return SUCCEEDED(hr);
    }
    T* operator=(IUnknown* newUnk) {
        if (ptr)
            ptr->Release();
        HRESULT hr = newUnk->QueryInterface(&ptr);
        if (FAILED(hr))
            ptr = nullptr;
        return ptr;
    }
    operator T*() const { return ptr; }
    T** operator&() { return &ptr; }
    T* operator->() const { return ptr; }
    T* operator=(T* newPtr) {
        if (ptr)
            ptr->Release();
        return (ptr = newPtr);
    }
};

template <typename T>
class ScopedGdiObj {
    T obj;

  public:
    explicit ScopedGdiObj(T obj) : obj(obj) {}
    ~ScopedGdiObj() { DeleteObject(obj); }
    operator T() const { return obj; }
};
typedef ScopedGdiObj<HFONT> ScopedFont;
typedef ScopedGdiObj<HPEN> ScopedPen;
typedef ScopedGdiObj<HBRUSH> ScopedBrush;

class ScopedHDC {
    HDC hdc;

  public:
    explicit ScopedHDC(HDC hdc) : hdc(hdc) {}
    ~ScopedHDC() { DeleteDC(hdc); }
    operator HDC() const { return hdc; }
};

class ScopedHdcSelect {
    HDC hdc;
    HGDIOBJ prev;

  public:
    ScopedHdcSelect(HDC hdc, HGDIOBJ obj) : hdc(hdc) { prev = SelectObject(hdc, obj); }
    ~ScopedHdcSelect() { SelectObject(hdc, prev); }
};

class ScopedCom {
  public:
    ScopedCom() { (void)CoInitialize(nullptr); }
    ~ScopedCom() { CoUninitialize(); }
};

class ScopedOle {
  public:
    ScopedOle() { (void)OleInitialize(nullptr); }
    ~ScopedOle() { OleUninitialize(); }
};

class ScopedGdiPlus {
  protected:
    Gdiplus::GdiplusStartupInput si;
    Gdiplus::GdiplusStartupOutput so;
    ULONG_PTR token, hookToken;
    bool noBgThread;

  public:
    // suppress the GDI+ background thread when initiating in WinMain,
    // as that thread causes DDE messages to be sent too early and
    // thus causes unexpected timeouts
    explicit ScopedGdiPlus(bool inWinMain = false) : noBgThread(inWinMain) {
        si.SuppressBackgroundThread = noBgThread;
        Gdiplus::GdiplusStartup(&token, &si, &so);
        if (noBgThread)
            so.NotificationHook(&hookToken);
    }
    ~ScopedGdiPlus() {
        if (noBgThread)
            so.NotificationUnhook(hookToken);
        Gdiplus::GdiplusShutdown(token);
    }
};
#endif
