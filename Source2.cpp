int main() 
{
	int n, i, j;
	cin >> n;
	int k = 0;
	for (i = 1, i <= n, i++)
	{
		for (j = 1, j <= n, j++)
		{
			cout << (k++) << ' ';
		}
		cout << endl;
	}
	return 0;
}