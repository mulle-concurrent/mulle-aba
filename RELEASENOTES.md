### 3.1.4








* storage allocation APIs now assert on allocation failure instead of returning NULL; callers must not rely on NULL returns (**BREAKING**)
* free/owned-pointer code now asserts on unexpected allocation failures instead of returning error codes, preventing silent continuation and possible NULL dereferences
