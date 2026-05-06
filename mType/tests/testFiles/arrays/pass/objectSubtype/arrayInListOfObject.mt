// MYT-281: a List<Object> accepts arrays as elements. The Object[] backing
// store of ArrayList<Object> stores the array Value heterogeneously after
// the MYT-281 widening; round-tripping back through cast preserves the
// element data.
import * from "../../../lib/collections/ArrayList.mt";

print("Testing array in List<Object>");

ArrayList<Object> bag = new ArrayList<Object>();

int[] ints = new int[2];
ints[0] = 100;
ints[1] = 200;

string[] strs = new string[2];
strs[0] = "foo";
strs[1] = "bar";

bag.add(ints);
bag.add(strs);

print("Bag size: " + bag.size());

Object first = bag.get(0);
int[] firstBack = (int[])first;
print("First element [0]: " + firstBack[0]);
print("First element [1]: " + firstBack[1]);

Object second = bag.get(1);
string[] secondBack = (string[])second;
print("Second element [0]: " + secondBack[0]);
print("Second element [1]: " + secondBack[1]);

print("Done");
