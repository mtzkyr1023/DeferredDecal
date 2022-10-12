

Texture2DArray cubeMap : register(t0);


float Factorial(int n) {
	int nm = n;
	float Out = n;
	if (nm == 0) {
		return 1;
	}
	else if (nm == 1) {
		return 1;
	}
	while (nm > 1) {
		nm = nm - 1;
		Out *= nm;
	}
	return Out;
}

float Pnpm(int n, int m, float ct) {
	if (n == 0 && m==0) {
		return 1.0;
	}
	else if (n == 1 && m == 0) {
		return ct;
	}
	else if (n == 1 && m == 1) {
		return sqrt(1 - ct * ct);
	}
	else if (n == 2 && m == 1) {
		return 3 * ct * sqrt(1 - ct * ct);
	}
	else {
		int nm = n - 1;
		return ((2 * nm + 1) *ct* Pnpm(nm, m, ct) - (nm + m) * Pnpm(nm - 1,m, ct)) /float(nm - m + 1);
	}
}

float Pnn(int n, float ct) {
	return Factorial(2 * n) * pow(1 - ct * ct, 0.5 * n) / (pow(2, n) * Factorial(n));
}



[numthreads(8,8,8)]
void main(uint3 id : SV_DispatchThreadID) {
	
}