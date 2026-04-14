// Annotation.mt - Annotation reflection wrapper
// Part of the mType reflection API

class Annotation {
    private int _nativeHandle;

    public constructor(int handle) {
        this._nativeHandle = handle;
    }

    // Get the annotation name
    public function getName(): string {
        string[] params = __reflect_getAnnotationParams(this._nativeHandle);
        // The name is typically stored as a special parameter or derived from handle
        // For now, return empty - needs native implementation
        return "";
    }

    // Get a parameter value by key (legacy string-only accessor; preserved for back-compat)
    public function getParameter(string key): string {
        return __reflect_getAnnotationParam(this._nativeHandle, key);
    }

    // Check if annotation has a parameter
    public function hasParameter(string key): bool {
        return __reflect_hasAnnotationParam(this._nativeHandle, key);
    }

    // Get all parameter keys
    public function getParameterKeys(): string[] {
        return __reflect_getAnnotationParams(this._nativeHandle);
    }

    // ===== MYT-108: typed parameter accessors =====
    // Each typed getter throws at the native layer if the underlying value's
    // declared type does not match. Use isNull(key) to probe nullable params.

    public function getInt(string key): int {
        return __reflect_getAnnotationInt(this._nativeHandle, key);
    }

    public function getFloat(string key): float {
        return __reflect_getAnnotationFloat(this._nativeHandle, key);
    }

    public function getBool(string key): bool {
        return __reflect_getAnnotationBool(this._nativeHandle, key);
    }

    public function getString(string key): string {
        return __reflect_getAnnotationString(this._nativeHandle, key);
    }

    public function getClass(string key): Class {
        int handle = __reflect_getAnnotationClass(this._nativeHandle, key);
        return new Class(handle);
    }

    public function isNull(string key): bool {
        return __reflect_isAnnotationParamNull(this._nativeHandle, key);
    }

    // Get the native handle
    public function getNativeHandle(): int {
        return this._nativeHandle;
    }
}
