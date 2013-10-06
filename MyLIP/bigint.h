#include <vector>
#include <string>
#include <iostream>
#include <assert.h>

typedef unsigned int uint32;
typedef int int32;
typedef unsigned long long uint64;
typedef long long int64;

struct BigUInt;
//前向声明
BigUInt BigDiv(const BigUInt& X,const BigUInt& Y,BigUInt &Q,BigUInt&R);
BigUInt Fast_BigDiv(const BigUInt& X,const BigUInt& Y,BigUInt&Q,BigUInt&R);
BigUInt GCD(const BigUInt& X,const BigUInt& Y);
BigUInt ExEuc(const BigUInt&a,const BigUInt& b,BigUInt& x,BigUInt& y);
BigUInt ExpMod(const BigUInt& a,const BigUInt& b,const BigUInt& m);

void ExEuclid( BigUInt a,BigUInt b,BigUInt& x,BigUInt&y );

//将大数表示为n^32 进制（即0x1 0000 0000）的数组

// 0xFFFFFFFF == 4294967295
// 0xFFFFFFFF **2 == FFFFFFFE00000001 ==  4294967295**2 == 18446744065119617025

// 0xFFFFFFFF FFFFFFFF == 18446744073709551615 ~= 4294967295.9999999998835846781731**2

struct BigUInt
{
	BigUInt():m_Sign(true)//true 为正，false为负
	{
		m_bits.resize(1);
		//if (m_bits.size() == 0)
		//{
			//m_bits.insert(m_bits.begin(),0);
		//	m_bits.push_back(0);
		//}
	}
	BigUInt(const uint32& ui32)
	{
		m_bits.push_back(ui32);
	}
	BigUInt(const uint64& ui64)
	{
		uint32 carry = ui64 / RADIX;
		uint32 v = ui64 % RADIX;
		m_bits.push_back(v);
		m_bits.push_back(carry);
	}
	BigUInt(const char * intstr)
	{
		Reset();
		FromString(*this,intstr);
	}
	
	BigUInt( const BigUInt& other)
	{
		m_bits = other.m_bits;
		m_Sign = other.m_Sign;
	}

	BigUInt& operator=(const BigUInt& other)
	{
		this->m_bits = other.m_bits;
		this->m_Sign = other.m_Sign;
		return *this;
	}
	
	std::string ToString() const;

	bool IsPositive() { return m_Sign;}
	const static uint64 RADIX =		0x0000000100000000;
	const static uint64 CARDINAL =	0x00000000ffffffff;

	const static bool NeedCarry(uint64 result);
	
	int Length()const;
	int ValidLength() const;
	//去掉从最高位的连续为0的元素
	void TrimHiZeros();
	void Reset()
	{
		m_bits.clear();
		m_bits.push_back(0);
	}
	
	//获得最高为1的比特的索引 N N-1 ... 3 2 1 0
	int32 GetNonZeroBitIdx(); //get_non_zero_bit_idx;

	//获取高位第一个不为零的uint32块的索引
	int32 BigUInt::GetNonZeroIdx()const;
	//获取从idx_hi,到idx_lo之间的数据构成的BigInt，用于除法的试除
	BigUInt GetBitRangBigInt(int bit_idx_hi,int bit_idx_lo) const;
	//比较大小 
	/*
		00000000 00001111 11110000 0000111
		         00000001 00001110 0011001
	*/
	int32 Compare(const BigUInt & N) const
	{
		int32 idx1 = this->ValidLength() - 1;
		int32 idx2 = N.ValidLength() - 1;

		uint32 v1 = 0;
		uint32 v2 = 0;
		
		//找到第一个不为0 的高位的值和索引
		while( !(v1=this->GetRadixBits(idx1)) && idx1>0)
		{ 
			idx1--;
		}

		while( !(v2 = N.GetRadixBits(idx2)) && idx2>0 )
		{ 
			idx2--;
		}
		int32 result = 0;
		while (idx1>=0 || idx2 >=0)
		{	
			if (idx1>idx2)
			{
				result = 1;
				break;
			}
			else if (idx2>idx1)
			{
				result = -1;
				break;
			}
			else  //idx1 == idx2
			{
				if (v1>v2)
				{
					result = 1;
					break;
				}
				else if (v2>v1)
				{
					result = -1;
					break;
				}
				else // idx1 == idx2 并且 v1 == v2，要判断下一位
				{
					idx1--;
					idx2--;
					v1 = this->GetRadixBits(idx1);
					v2 = N.GetRadixBits(idx2);
				}
			}
		}
		return result;
	}

	friend int operator < (const BigUInt& v1,const BigUInt& v2)
	{
		return v1.Compare(v2) == -1;
	}

	friend int operator == (const BigUInt& v1,const BigUInt& v2)
	{
		return v1.Compare(v2) == 0;
	}

	friend int operator > (const BigUInt& v1,const BigUInt& v2)
	{
		return v1.Compare(v2) == 1;
	}

	friend int operator <= (const BigUInt& v1,const BigUInt& v2)
	{
		return !(v1>v2);
	}

	friend int operator >= (const BigUInt& v1,const BigUInt& v2)
	{
		return !(v1<v2);
	}

	friend int operator !=(const BigUInt& v1,const BigUInt& v2)
	{
		return !(v1==v2);
	}

	//乘以一个小整数
	// a b c d
	//*      v
	//---------
	         
	void Mul(const uint32& ui32) ;
	
	void Add(const uint32& ui32) ;
	
	//
	uint32 Mod(const uint32& ui32)
	{
		uint32 index = m_bits.size() - 1;
		
		uint64 v = 0;
		uint64 quotient = 0;
		uint64 remainder = 0;
		do 
		{
			v = remainder*0x100000000 + m_bits[index];
			
			quotient = v / ui32;
			remainder = v % ui32;
		} while (--index>=0);
		return remainder;
	}
	void Dump(const char * msg="",...) const;
	//获取pos位的uint32的数
	//下标是 n n-1 n-2 ... 2 1 0 对应uint32数组
	//由于uint32 数组 是 0 1 2 ... n-1 n，并且为了使用vector时的效率（在头部插入）
	inline uint32 GetRadixBits(uint32 pos) const
	{
		if(pos<0 || pos >=m_bits.size() )
			return 0;
		return m_bits[pos];
	}

	inline void SetRadixBits(uint32 v,uint32 pos)
	{
		//assert(pos);
		if (pos>=m_bits.size())
		{
			//m_bits.resize(pos+1,0);
			m_bits.resize(pos+1,0);
		}
		m_bits[pos] = v;
	}

	void AddRadixBits(uint32 v,uint32 pos);

	void MinusRadixBits(uint32 v,uint32 pos);
	

	typedef std::vector<uint32> BigIntBitType;

	//位置:  4 3 2 1 0
	//数字：| e...e | d...d | c...c | b...b | a...a |
	BigIntBitType m_bits;
	bool m_Sign;


	friend	void FromString(BigUInt& N,const std::string& numbers)
	{
		int length = 0;
		for(;(!numbers.empty())&&numbers[length]!='\0';length++);

		int base = 10;
		if (numbers[0]=='0' && (numbers[1]== 'x' || numbers[1]== 'X'))
		{
			base = 16;
		}
		
		//N.Dump();
		switch (base)
		{
		case 10:
			{
				for (int i=0;i<length;i++)
				{
					int n = numbers[i]-'0';
					N.Mul(10);
					//N.Dump();
					N.Add(n);
				}
			}
			break;
		case 16:
			{
				for (int i=2;i<length;i++)
				{
					int n = numbers[i];
					if (n==' ')
					{
						continue;
					}
					N.Mul(16);
					
					switch(n)
					{
					case 'a':
					case 'A':
						N.Add(10);
						break;
					case 'b':
					case 'B':
						N.Add(11);
						break;
					case 'c':
					case 'C':
						N.Add(12);
						break;
					case 'd':
					case 'D':
						N.Add(13);
						break;
					case 'e':
					case 'E':
						N.Add(14);
						break;
					case 'f':
					case 'F':
						N.Add(15);
						break;
					//for 0~9
					default:
						{
							n = n -'0';
							assert(n>=0 && n<=9);
							N.Add(n);
						}
					}
				}
			}
			break;
		}
	}
	friend BigUInt Add(const BigUInt& N1,const BigUInt& N2)
	{
		BigUInt R;
		

		uint64 carry=0;
		uint32 index=0;
		for (;index<N1.ValidLength() || index < N2.ValidLength();index++)
		{
			uint32 n1 = N1.GetRadixBits(index);
			uint32 n2 = N2.GetRadixBits(index);
			uint64 v = n1+n2+carry;

			carry = v / RADIX;
			v = v % RADIX;
			R.AddRadixBits((uint32)v,index);
			R.AddRadixBits((uint32)carry,index+1);
		}
		R.TrimHiZeros();
		return R;
	}
	
	friend BigUInt operator+(const BigUInt& X,const BigUInt& Y);

	//N1 > N2,那么N1长度大于等于N2长度
	friend BigUInt Minus(const BigUInt& N1,const BigUInt& N2)
	{
		BigUInt R=N1;
		
		for (int index=0;index<N2.ValidLength();index++)
		{
			uint32 v = N2.GetRadixBits(index);
			R.MinusRadixBits(v,index);
		}
		R.TrimHiZeros();
		return R;
	}

	friend BigUInt operator-(const BigUInt&N1,const BigUInt& N2)
	{
		return Minus(N1,N2);
	}

	/*
							a		b		c   :index1 0~2
					  d		e		f		g   :index2 0~3
					  ------------------------
						   a*g     b*g     c*g
					 a*f   b*f     c*f           
			 a*e     b*e   c*e 
	 a*d     b*d     c*d

	 */
	friend  BigUInt Mul(const BigUInt& N1,const BigUInt& N2)
	{
			BigUInt R;
			
			uint32 carry = 0;

			uint64 v = 0;
			int len1 = N1.ValidLength();
			int len2 = N2.ValidLength();
			for (uint32 index1 = 0;index1<len1;index1++)
			{
				uint64 n1 = N1.GetRadixBits(index1);
				if (n1==0)
				{
					continue;
				}
				for (uint32 index2 = 0;index2<len2;index2++)
				{
					uint64 n2 = N2.GetRadixBits(index2);
					if (n2==0)
					{
						continue;
					}
					//注意这里转型操作，否则v里的结果是32位的
					v = (uint64)n1*(uint64)n2 ;
					carry = v / RADIX;
					v = v % RADIX;

					//把值和进位加到结果中
					R.AddRadixBits(v,index1+index2);
					R.AddRadixBits(carry,index1+index2+1);
				}
			}
		R.TrimHiZeros();
		return R;
	};

	friend BigUInt operator*(const BigUInt& N1,const BigUInt& N2)
	{
		return ::Mul(N1,N2);
	}


	friend BigUInt Mod(const BigUInt& X,const BigUInt& M)
	{
		BigUInt q;//商
		BigUInt r;//余
		Fast_BigDiv(X,M,q,r);
		return r;
	}

	friend BigUInt operator% (const BigUInt&X,const BigUInt& Y)
	{
		return ::Mod(X,Y);
	}

	friend BigUInt operator>>(const BigUInt& X,int bits)
	{
		BigUInt result = X,zero;

		if (bits>=X.ValidLength()*4*8)
		{
			return zero;
		}

		int complete_ints = bits/32;
		int remaind = bits%32;

		//int from_hi_ints = X.ValidLength() - complete_ints;

		//for (int i=0;i<from_hi_ints;i++)
		//{
		//	uint32 v = X.GetRadixBits(X.ValidLength() - i - 1);
		//	result.SetRadixBits(v,from_hi_ints - i - 1);
		//}
		
		if (complete_ints>0)
		{
			result.m_bits.erase(result.m_bits.begin(),result.m_bits.begin() + complete_ints);
		}


		
		//这里可以优化掉，当remaind	> 0 才进行下面的操作，也就不存在int移动32bit之后期待为0，但是值不变的现象

		// 11110000 11110000 11110000 11110000 
		
		//右移一位
		// 01111000 01111000 01111000 01111000

		//向右移动remaind位，hi_mask是 31 ~ remaind ，lo_mask 是 remaind ~ 0位
		uint32 hi_mask = 0xffffffff << remaind;
		uint32 lo_mask = (remaind == 0) ? 0 : 0xffffffff >>(32 - remaind);


		
		for (int j=0;remaind && j<result.ValidLength();j++)
		{
			uint32 hi_v=result.GetRadixBits(j+1);
			
			uint32 bits_from_hi = hi_v & lo_mask; // 高一位的

			uint32 v = result.GetRadixBits(j);


			uint32 bits_from_lo = v &  hi_mask;

			//高位的bits移到低位的 高bit部分，再和原来低位的搞bit部分组成新的值
			uint32 new_v = (bits_from_hi << (32-remaind) )  | (bits_from_lo>>remaind);

			result.SetRadixBits(new_v,j);
		}
		result.TrimHiZeros();
		return result;

	}

	// 11111111000000001111111100000000 11110000
	//左移一位后:
	// 11111110000000011111111000000001 11100000
	friend BigUInt operator<<(const BigUInt& X,int bits)
	{
		BigUInt result=X;

		int complete_ints = bits/32;
		int remaind = bits%32; //remaind 0~31
		
		//32位整数这样移动
		//for(int idx=0;idx<X.Length();idx++)
		//{
		//	uint32 v = X.GetRadixBits(idx);
		//	result.SetRadixBits(v,idx+complete_ints);
		//}

		if (complete_ints>0)
		{
			//在vector头部（相当于在数的右边）加上complete_ints个0，也就是左移了complete_ints*32位
			result.m_bits.insert(result.m_bits.begin(),complete_ints,0);
		}
		/*
		if (complete_ints>0)
		{
		//先插入0，在插入真正的数据，要比先插入数据，然后在头部插入0要快
		result.m_bits.insert(result.m_bits.begin(),complete_ints,0);
		result.m_bits.insert(result.m_bits.end(),X.m_bits.begin(),X.m_bits.end());
		}
		*/

		//移动零散的bit:
		uint32 hi_mask = (remaind==0) ? 0 : (0xffffffff << (32 - remaind)); //因为remaind为0时，值v左移32位仍然是v
		uint32 lo_mask = 0xffffffff >> remaind;
		
		for(int idx = result.ValidLength();remaind && idx>=0;idx--)
		{
			uint32 hi_v = result.GetRadixBits(idx);
			uint32 lo_v = result.GetRadixBits(idx-1);

			uint32 new_hi_v = ( (hi_v & lo_mask) << remaind ) | ( ( lo_v & hi_mask ) >> (32 - remaind) );
			if (new_hi_v) //不随意添加额外的空白uint32
			{
				result.SetRadixBits(new_hi_v,idx);
			}
		}

		result.TrimHiZeros();
		return result;
	}
	
	//大数除法：
	//返回两个大数除法的商，同时Q置为商，R为余数
	friend BigUInt BigDiv(const BigUInt& X,const BigUInt& Y,BigUInt &Q,BigUInt&R);
	friend BigUInt Fast_BigDiv(const BigUInt& X,const BigUInt& Y,BigUInt&Q,BigUInt&R);
	friend BigUInt operator/ (const BigUInt& X,const BigUInt& Y)
	{
		BigUInt Q;
		BigUInt R;
		return Fast_BigDiv(X,Y,Q,R);
	}

	//欧几里德算法，求X和Y的最大公约数
	friend BigUInt GCD(const BigUInt& X,const BigUInt& Y);

	//扩展欧几里德算法，求模反元素
	friend void ExEuclid(BigUInt a, BigUInt b,BigUInt& x,BigUInt&y); 
};
