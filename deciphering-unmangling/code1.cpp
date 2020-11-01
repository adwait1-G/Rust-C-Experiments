#include <iostream>
#include <string>

using namespace std;

namespace c1
{

int add (int a, int b)
{
	return (a+b);
}

int add (int a, int b, int c)
{
	return (a+b+c);
}

string add (string s1, string s2)
{
	return (s1+s2);
}


} /* namespace c1 */


int main()
{
	int sum1 = c1::add(1, 2);
	int sum2 = c1::add(2, 3, 4);
	
	string sum3 = c1::add("Some-string1", "Some-string2");

	cout<<sum1<<", "<<sum2<<", "<<sum3<<endl;

	return 0;
}
