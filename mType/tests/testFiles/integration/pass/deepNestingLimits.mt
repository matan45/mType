// Test deep nesting scenarios
namespace level1 {
    final int VALUE1 = 1;
    
    namespace level2 {
        final int VALUE2 = 2;
        
        namespace level3 {
            final int VALUE3 = 3;
            
            namespace level4 {
                final int VALUE4 = 4;
                
                namespace level5 {
                    final int VALUE5 = 5;
                    
                    class DeepClass {
                        int depth;
                        
                        constructor() {
                            depth = VALUE1 + VALUE2 + VALUE3 + VALUE4 + VALUE5;
                        }
                        
                        function getDepthInfo(): string {
                            string info = "Depth: " + toString(depth);
                            
                            // Deep nested control flow
                            for (int i = 0; i < 3; i++) {
                                for (int j = 0; j < 2; j++) {
                                    for (int k = 0; k < 2; k++) {
                                        if (i + j + k == 3) {
                                            info = info + " [Special]";
                                        }
                                    }
                                }
                            }
                            
                            return info;
                        }
                    }
                }
            }
        }
    }
}

// Test deep namespace access
level1::level2::level3::level4::level5::DeepClass deepObj = new level1::level2::level3::level4::level5::DeepClass();
print(deepObj.getDepthInfo());

// Test using directive with deep nesting
using namespace level1::level2::level3::level4::level5;
DeepClass anotherDeep = new DeepClass();
print(anotherDeep.getDepthInfo());