import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/collections/HashMap.mt";
import * from "../../lib/primitives/String.mt";

// A dynamic JSON tree, used by LspClient because mType's class-driven
// JSON deserializer doesn't fit LSP's variable result/params shapes.
// Construct outbound trees with the static factory methods + setters,
// then call toJsonString(). Parse inbound text with JsonNode::parse(text)
// and walk the result via getString/getInt/getArray/getObject.
//
// Type tags:
//   0 = null, 1 = bool, 2 = int, 3 = float, 4 = string, 5 = array, 6 = object
class JsonNode {
    public int kind;
    public bool boolVal;
    public int intVal;
    public float floatVal;
    public string strVal;
    public ArrayList<JsonNode> arrVal;
    // Keyed by boxed String — primitive `string` isn't allowed as a
    // generic type argument in mType. Convert at the public boundary
    // (set/get/has) so callers keep working in raw strings.
    public HashMap<String, JsonNode> objVal;

    public constructor(int kind) {
        this.kind = kind;
        this.boolVal = false;
        this.intVal = 0;
        this.floatVal = 0.0;
        this.strVal = "";
        this.arrVal = new ArrayList<JsonNode>();
        this.objVal = new HashMap<String, JsonNode>();
    }

    public static function nullNode(): JsonNode {
        return new JsonNode(0);
    }
    public static function boolNode(bool v): JsonNode {
        JsonNode n = new JsonNode(1);
        n.boolVal = v;
        return n;
    }
    public static function intNode(int v): JsonNode {
        JsonNode n = new JsonNode(2);
        n.intVal = v;
        return n;
    }
    public static function floatNode(float v): JsonNode {
        JsonNode n = new JsonNode(3);
        n.floatVal = v;
        return n;
    }
    public static function strNode(string v): JsonNode {
        JsonNode n = new JsonNode(4);
        n.strVal = v;
        return n;
    }
    public static function arrayNode(): JsonNode {
        return new JsonNode(5);
    }
    public static function objectNode(): JsonNode {
        return new JsonNode(6);
    }

    public function isNull(): bool   { return this.kind == 0; }
    public function isBool(): bool   { return this.kind == 1; }
    public function isInt(): bool    { return this.kind == 2; }
    public function isFloat(): bool  { return this.kind == 3; }
    public function isString(): bool { return this.kind == 4; }
    public function isArray(): bool  { return this.kind == 5; }
    public function isObject(): bool { return this.kind == 6; }

    // Object accessors. All return safe defaults when the key is missing
    // or the type is wrong, so callers can chain without null checks.
    public function has(string key): bool {
        if (!this.isObject()) return false;
        return this.objVal.containsKey(new String(key));
    }
    public function get(string key): JsonNode {
        if (!this.isObject()) return JsonNode::nullNode();
        String boxed = new String(key);
        if (!this.objVal.containsKey(boxed)) return JsonNode::nullNode();
        return this.objVal.get(boxed);
    }
    public function getString(string key): string {
        JsonNode child = this.get(key);
        if (child.isString()) return child.strVal;
        return "";
    }
    public function getInt(string key): int {
        JsonNode child = this.get(key);
        if (child.isInt()) return child.intVal;
        if (child.isFloat()) return (int)child.floatVal;
        return 0;
    }
    public function getBool(string key): bool {
        JsonNode child = this.get(key);
        if (child.isBool()) return child.boolVal;
        return false;
    }
    public function getArray(string key): JsonNode {
        JsonNode child = this.get(key);
        if (child.isArray()) return child;
        return JsonNode::arrayNode();
    }
    public function getObject(string key): JsonNode {
        JsonNode child = this.get(key);
        if (child.isObject()) return child;
        return JsonNode::objectNode();
    }

    // Mutators for outbound construction.
    public function set(string key, JsonNode value): void {
        if (!this.isObject()) return;
        this.objVal.put(new String(key), value);
    }
    public function setString(string key, string value): void {
        this.set(key, JsonNode::strNode(value));
    }
    public function setInt(string key, int value): void {
        this.set(key, JsonNode::intNode(value));
    }
    public function setBool(string key, bool value): void {
        this.set(key, JsonNode::boolNode(value));
    }
    public function add(JsonNode value): void {
        if (!this.isArray()) return;
        this.arrVal.add(value);
    }

    // Outbound serialization. Compact (no whitespace) — LSP doesn't care.
    public function toJsonString(): string {
        if (this.isNull())   return "null";
        if (this.isBool())   { if (this.boolVal) return "true"; return "false"; }
        if (this.isInt())    return "" + this.intVal;
        if (this.isFloat())  return "" + this.floatVal;
        if (this.isString()) return JsonNode::escapeString(this.strVal);
        if (this.isArray()) {
            string s = "[";
            for (int i = 0; i < this.arrVal.size(); i = i + 1) {
                if (i > 0) s = s + ",";
                s = s + this.arrVal.get(i).toJsonString();
            }
            return s + "]";
        }
        if (this.isObject()) {
            string s = "{";
            String[] keys = this.objVal.getKeys();
            for (int i = 0; i < keys.length; i = i + 1) {
                if (i > 0) s = s + ",";
                String k = keys[i];
                s = s + JsonNode::escapeString(k.getValue()) + ":" +
                    this.objVal.get(k).toJsonString();
            }
            return s + "}";
        }
        return "null";
    }

    private static function escapeString(string s): string {
        string out = "\"";
        int n = strLength(s);
        int i = 0;
        while (i < n) {
            string c = substring(s, i, 1);
            if (c == "\"") {
                out = out + "\\\"";
            } else if (c == "\\") {
                out = out + "\\\\";
            } else if (c == "\n") {
                out = out + "\\n";
            } else if (c == "\r") {
                out = out + "\\r";
            } else if (c == "\t") {
                out = out + "\\t";
            } else {
                out = out + c;
            }
            i = i + 1;
        }
        return out + "\"";
    }

    // Recursive-descent parser. Returns a null node on malformed input
    // rather than throwing — LspClient logs and skips bad messages.
    public static function parse(string text): JsonNode {
        JsonCursor cur = new JsonCursor(text);
        cur.skipWs();
        return JsonNode::parseValue(cur);
    }

    public static function parseValue(JsonCursor cur): JsonNode {
        cur.skipWs();
        if (cur.eof()) return JsonNode::nullNode();
        string c = cur.peek();
        if (c == "{") return JsonNode::parseObject(cur);
        if (c == "[") return JsonNode::parseArray(cur);
        if (c == "\"") return JsonNode::parseString(cur);
        if (c == "t" || c == "f") return JsonNode::parseBool(cur);
        if (c == "n") return JsonNode::parseNull(cur);
        return JsonNode::parseNumber(cur);
    }

    private static function parseObject(JsonCursor cur): JsonNode {
        JsonNode obj = JsonNode::objectNode();
        cur.advance();
        cur.skipWs();
        if (!cur.eof() && cur.peek() == "}") {
            cur.advance();
            return obj;
        }
        while (!cur.eof()) {
            cur.skipWs();
            JsonNode keyNode = JsonNode::parseString(cur);
            cur.skipWs();
            if (cur.eof() || cur.peek() != ":") return obj;
            cur.advance();
            JsonNode value = JsonNode::parseValue(cur);
            obj.set(keyNode.strVal, value);
            cur.skipWs();
            if (cur.eof()) return obj;
            string c = cur.peek();
            if (c == ",") {
                cur.advance();
                continue;
            }
            if (c == "}") {
                cur.advance();
                return obj;
            }
            return obj;
        }
        return obj;
    }

    private static function parseArray(JsonCursor cur): JsonNode {
        JsonNode arr = JsonNode::arrayNode();
        cur.advance();
        cur.skipWs();
        if (!cur.eof() && cur.peek() == "]") {
            cur.advance();
            return arr;
        }
        while (!cur.eof()) {
            JsonNode value = JsonNode::parseValue(cur);
            arr.add(value);
            cur.skipWs();
            if (cur.eof()) return arr;
            string c = cur.peek();
            if (c == ",") {
                cur.advance();
                continue;
            }
            if (c == "]") {
                cur.advance();
                return arr;
            }
            return arr;
        }
        return arr;
    }

    private static function parseString(JsonCursor cur): JsonNode {
        if (cur.eof() || cur.peek() != "\"") return JsonNode::strNode("");
        cur.advance();
        string result = "";
        while (!cur.eof()) {
            string c = cur.peek();
            if (c == "\"") {
                cur.advance();
                return JsonNode::strNode(result);
            }
            if (c == "\\") {
                cur.advance();
                if (cur.eof()) return JsonNode::strNode(result);
                string esc = cur.peek();
                cur.advance();
                if (esc == "\"")      result = result + "\"";
                else if (esc == "\\") result = result + "\\";
                else if (esc == "/")  result = result + "/";
                else if (esc == "n")  result = result + "\n";
                else if (esc == "r")  result = result + "\r";
                else if (esc == "t")  result = result + "\t";
                else if (esc == "b")  result = result + "\b";
                else if (esc == "f")  result = result + "\f";
                else if (esc == "u") {
                    // Skip 4 hex digits — we don't decode \uXXXX into
                    // UTF-8 because mType has no charFromCode primitive.
                    // Real LSP servers emit non-ASCII directly as UTF-8;
                    // \u escapes are rare and dropping them keeps the
                    // string mostly intact.
                    if (cur.remaining() >= 4) {
                        cur.advanceN(4);
                    }
                }
                else result = result + esc;
            } else {
                result = result + c;
                cur.advance();
            }
        }
        return JsonNode::strNode(result);
    }

    private static function parseBool(JsonCursor cur): JsonNode {
        if (cur.peek() == "t") {
            if (cur.remaining() >= 4 && cur.peekRange(4) == "true") {
                cur.advanceN(4);
                return JsonNode::boolNode(true);
            }
        } else {
            if (cur.remaining() >= 5 && cur.peekRange(5) == "false") {
                cur.advanceN(5);
                return JsonNode::boolNode(false);
            }
        }
        return JsonNode::nullNode();
    }

    private static function parseNull(JsonCursor cur): JsonNode {
        if (cur.remaining() >= 4 && cur.peekRange(4) == "null") {
            cur.advanceN(4);
        }
        return JsonNode::nullNode();
    }

    private static function parseNumber(JsonCursor cur): JsonNode {
        int start = cur.pos;
        if (!cur.eof() && cur.peek() == "-") cur.advance();
        while (!cur.eof() && JsonNode::isDigit(cur.peek())) cur.advance();
        bool isFloat = false;
        if (!cur.eof() && cur.peek() == ".") {
            isFloat = true;
            cur.advance();
            while (!cur.eof() && JsonNode::isDigit(cur.peek())) cur.advance();
        }
        if (!cur.eof() && (cur.peek() == "e" || cur.peek() == "E")) {
            isFloat = true;
            cur.advance();
            if (!cur.eof() && (cur.peek() == "+" || cur.peek() == "-")) cur.advance();
            while (!cur.eof() && JsonNode::isDigit(cur.peek())) cur.advance();
        }
        string raw = substring(cur.text, start, cur.pos - start);
        if (isFloat) {
            return JsonNode::floatNode(JsonNode::parseFloatStr(raw));
        }
        return JsonNode::intNode(JsonNode::parseIntStr(raw));
    }

    private static function isDigit(string c): bool {
        return c == "0" || c == "1" || c == "2" || c == "3" || c == "4"
            || c == "5" || c == "6" || c == "7" || c == "8" || c == "9";
    }

    private static function digitValue(string c): int {
        if (c == "0") return 0;
        if (c == "1") return 1;
        if (c == "2") return 2;
        if (c == "3") return 3;
        if (c == "4") return 4;
        if (c == "5") return 5;
        if (c == "6") return 6;
        if (c == "7") return 7;
        if (c == "8") return 8;
        if (c == "9") return 9;
        return 0;
    }

    private static function parseIntStr(string s): int {
        int n = strLength(s);
        if (n == 0) return 0;
        int i = 0;
        bool neg = false;
        if (substring(s, 0, 1) == "-") {
            neg = true;
            i = 1;
        }
        int v = 0;
        while (i < n) {
            v = v * 10 + JsonNode::digitValue(substring(s, i, 1));
            i = i + 1;
        }
        if (neg) return -v;
        return v;
    }

    private static function parseFloatStr(string s): float {
        // Split into mantissa, fractional part, and exponent. We don't
        // need bit-perfect IEEE rounding here — LSP positions are integers,
        // any float fields are coarse hints (timeouts, percentages).
        int n = strLength(s);
        if (n == 0) return 0.0;
        int i = 0;
        bool neg = false;
        if (substring(s, 0, 1) == "-") {
            neg = true;
            i = 1;
        } else if (substring(s, 0, 1) == "+") {
            i = 1;
        }
        float v = 0.0;
        while (i < n && JsonNode::isDigit(substring(s, i, 1))) {
            v = v * 10.0 + (float)JsonNode::digitValue(substring(s, i, 1));
            i = i + 1;
        }
        if (i < n && substring(s, i, 1) == ".") {
            i = i + 1;
            float frac = 0.0;
            float scale = 1.0;
            while (i < n && JsonNode::isDigit(substring(s, i, 1))) {
                frac = frac * 10.0 + (float)JsonNode::digitValue(substring(s, i, 1));
                scale = scale * 10.0;
                i = i + 1;
            }
            v = v + (frac / scale);
        }
        if (i < n && (substring(s, i, 1) == "e" || substring(s, i, 1) == "E")) {
            i = i + 1;
            bool expNeg = false;
            if (i < n && substring(s, i, 1) == "-") { expNeg = true; i = i + 1; }
            else if (i < n && substring(s, i, 1) == "+") { i = i + 1; }
            int exp = 0;
            while (i < n && JsonNode::isDigit(substring(s, i, 1))) {
                exp = exp * 10 + JsonNode::digitValue(substring(s, i, 1));
                i = i + 1;
            }
            float p = 1.0;
            int j = 0;
            while (j < exp) {
                p = p * 10.0;
                j = j + 1;
            }
            if (expNeg) v = v / p;
            else v = v * p;
        }
        if (neg) return -v;
        return v;
    }
}

class JsonCursor {
    public string text;
    public int pos;
    public int len;

    public constructor(string text) {
        this.text = text;
        this.pos = 0;
        this.len = strLength(text);
    }

    public function eof(): bool {
        return this.pos >= this.len;
    }

    public function remaining(): int {
        return this.len - this.pos;
    }

    public function peek(): string {
        if (this.eof()) return "";
        return substring(this.text, this.pos, 1);
    }

    public function peekRange(int n): string {
        if (this.pos + n > this.len) return substring(this.text, this.pos, this.len - this.pos);
        return substring(this.text, this.pos, n);
    }

    public function advance(): void {
        this.pos = this.pos + 1;
    }

    public function advanceN(int n): void {
        this.pos = this.pos + n;
    }

    public function skipWs(): void {
        while (!this.eof()) {
            string c = this.peek();
            if (c == " " || c == "\t" || c == "\n" || c == "\r") {
                this.advance();
            } else {
                return;
            }
        }
    }
}