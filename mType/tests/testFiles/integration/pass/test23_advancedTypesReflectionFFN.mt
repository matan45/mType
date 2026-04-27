// Test 23: Advanced Types, Reflection, FFN
// Features: Reflection, generics, annotations, casting, FFN, import with alias.

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Annotation.mt";
import * from "../../lib/reflect/Constructor.mt";
import * from "../../lib/primitives/String.mt";

annotation Plugin {
    string name = "";
    string version = "";
}

interface IPlugin {
    function initialize(): void;
    function execute(string data): string;
}

// Simulated FFN boundary call
function __native_mock_ffn(string input): string {
    return "FFN_PROCESSED[" + input + "]";
}

// Generic plugin base
abstract class BasePlugin<T> implements IPlugin {
    protected T state;
    
    public constructor(T initialState) {
        this.state = initialState;
    }
    
    public function initialize(): void {
        print("BasePlugin initialized");
    }
    
    public abstract function execute(string data): string;
}

@Plugin(name="StringProcessor", version="1.0.0")
class StringPlugin extends BasePlugin<String> {
    
    public constructor() : super(new String("INIT")) {
    }
    
    public function execute(string data): string {
        string ffnResult = __native_mock_ffn(data);
        return "StringPlugin[" + this.state.getValue() + "]: " + ffnResult;
    }
}

function loadPluginDynamically(string className): IPlugin? {
    Class cls = Class::forName(className);
    
    if (cls.hasAnnotation("Plugin")) {
        Annotation? ann = cls.getAnnotation("Plugin");
        if (ann != null) {
            string pluginName = ann.getString("name");
            string pluginVersion = ann.getString("version");
            print("Found plugin: " + pluginName + " v" + pluginVersion);
            
            Constructor ctor = cls.getConstructor(0);
            Object[] args = new Object[0];
            Object instance = ctor.newInstance(args);
            
            if (instance isClassOf StringPlugin) {
                StringPlugin plugin = (StringPlugin)instance;
                return plugin;
            }
        }
    }
    
    return null;
}

function main(): void {
    print("--- Test 23: Advanced Types, Reflection, FFN ---");
    
    IPlugin? plugin = loadPluginDynamically("StringPlugin");
    
    if (plugin != null) {
        plugin.initialize();
        string result = plugin.execute("raw_data");
        print("Execution Result: " + result);
    }
}

main();