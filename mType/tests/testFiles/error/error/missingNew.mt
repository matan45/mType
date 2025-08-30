/ Another class with static utilities
class StringUtils {
	
    static function reverse(string str): string {
        // Simple reverse implementation for demo
        return str + " (reversed)";  // Simplified for demo
    }
    
    static function repeat(string str, int times): string {
        string result = "";
        for (int i = 0; i < times; i = i + 1) {
            result = result + str;
        }
        return result;
    }
}
StringUtils var = StringUtils()//need to faild missing new