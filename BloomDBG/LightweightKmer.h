#ifndef LIGHTWEIGHT_KMER_H
#define LIGHTWEIGHT_KMER_H 1

#include "BloomDBG/MaskedKmer.h"
#include "Common/Kmer.h"
#include "Common/Sequence.h"
#include "Common/Sense.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <boost/shared_array.hpp>

/**
 * Class that stores a shared pointer to a k-mer (char array).
 *
 * I implemented this class because I observed that storing and
 * copying the full char array between data structures was hurting
 * performance and using a lot of memory.
 *
 * Having a lightweight k-mer representation is particularly
 * important when using it as the `vertex_descriptor` in a Boost graph.
 */
class LightweightKmer
{
private:

	/** Shared pointer to k-mer data */
	boost::shared_array<char> m_kmer;

public:

	/** Default constructor */
	LightweightKmer() {}

	/** Constructor */
	LightweightKmer(const char* kmer) : m_kmer(new char[Kmer::length() + 1])
	{
		const unsigned k = Kmer::length();
		std::copy(kmer, kmer + k, m_kmer.get());
		/* null-terminate k-mer string */
		*(m_kmer.get() + k) = '\0';
	}

	/** Get pointer to raw char array for k-mer */
	char* c_str() { return (char*)m_kmer.get(); }

	/** Get pointer to raw char array for k-mer (read-only) */
	const char* c_str() const { return (const char*)m_kmer.get(); }

	/** Shift the k-mer and set the value of the new incoming base. */
	void shift(extDirection dir, char charIn = 'A')
	{
		const unsigned k = Kmer::length();
		assert(k >= 2);
		if (dir == SENSE) {
			memmove(m_kmer.get(), m_kmer.get() + 1, k - 1);
		} else {
			memmove(m_kmer.get() + 1, m_kmer.get(), k - 1);
		}
		setLastBase(dir, charIn);
	}

	/** Change the first/last base of the k-mer */
	void setLastBase(extDirection dir, char base)
	{
		const unsigned k = Kmer::length();
		unsigned pos = (dir == SENSE) ? k - 1 : 0;
		setBase(pos, base);
	}

	/** Change a base within the k-mer */
	void setBase(unsigned pos, char base)
	{
		assert(pos < Kmer::length());
		*(m_kmer.get() + pos) = base;
	}

	/** Get the base (ACGT) at a given position */
	char getBase(unsigned pos) const
	{
		return *(m_kmer.get() + pos);
	}

	/**
	 * Return true if k-mer is in its lexicographically smallest orientation
	 */
	bool isCanonical() const
	{
		const unsigned k = Kmer::length();
		for (unsigned i = 0; i < k/2; ++i) {
			const char c1 = toupper(m_kmer.get()[i]);
			const char c2 = complementBaseChar(
				toupper(m_kmer.get()[k-i-1]));
			if (c1 > c2)
				return false;
			else if (c1 < c2)
				return true;
		}
		return true;
	}

	/**
	 * Reverse complement the k-mer, if it is not already in its
	 * canonical (i.e. lexicographically smallest) orientation.
	 */
	void canonicalize()
	{
		if (!isCanonical())
			reverseComplement();

	}

	void reverseComplement()
	{
		const unsigned k = Kmer::length();
		for (unsigned i = 0; i < k/2; ++i) {
			const char tmp = complementBaseChar(m_kmer.get()[i]);
			m_kmer.get()[i] = complementBaseChar(
				m_kmer.get()[k-i-1]);
			m_kmer.get()[k-i-1] = tmp;
		}

		/* if k is odd, we need to reverse complement the middle char */
		if (k % 2 == 1 && k > 1)
			m_kmer.get()[k/2]
				= complementBaseChar(m_kmer.get()[k/2]);
	}

	/** Equality operator */
	bool operator==(const LightweightKmer& o) const
	{
		unsigned k = Kmer::length();
		const std::string& spacedSeed = MaskedKmer::mask();

		if (spacedSeed.empty()) {
			return !memcmp(m_kmer.get(), o.m_kmer.get(), k);
		} else {
			assert(spacedSeed.length() == k);
			for (unsigned i = 0; i < k; ++i) {
				if (spacedSeed.at(i) != '0' && getBase(i) != o.getBase(i))
					return false;
			}
			return true;
		}
	}

	/** Inequality operator */
	bool operator!=(const LightweightKmer& o) const
	{
		return !operator==(o);
	}

};

#endif
