#include "Assembly/Options.h"
#if PAIRED_DBG
#include "PairedDBG/PairedDBGAlgorithms.h"
#include "PairedDBG/SequenceCollection.h"
#else
#include "Assembly/SequenceCollection.h"
#endif
#include "Assembly/AssemblyAlgorithms.h"
#include "Assembly/DotWriter.h"
#include "DataBase/DB.h"

#include <algorithm>
#include <cstdio> // for setvbuf
#include <fstream>
#include <iostream>
#include <sstream>

using namespace std;

DB db;

static void
removeLowCoverageContigs(SequenceCollectionHash& g)
{
	AssemblyAlgorithms::markAmbiguous(&g);

	cout << "Removing low-coverage contigs "
	        "(mean k-mer coverage < "
	     << opt::coverage << ")\n";
	AssemblyAlgorithms::assemble(&g);
	AssemblyAlgorithms::splitAmbiguous(&g);

	opt::coverage = 0;
}

static void
popBubbles(SequenceCollectionHash& g)
{
	cout << "Popping bubbles" << endl;
	ofstream out;
	AssemblyAlgorithms::openBubbleFile(out);
	unsigned numPopped = AssemblyAlgorithms::popBubbles(&g, out);
	assert(out.good());
	cout << "Removed " << numPopped << " bubbles\n";
}

static void
write_graph(const string& path, const SequenceCollectionHash& c)
{
	if (path.empty())
		return;
	cout << "Writing graph to `" << path << "'\n";
	ofstream out(path.c_str());
	DotWriter::write(out, c);
}

static void
assemble(const string& pathIn, const string& pathOut)
{
	Timer timer(__func__);
	SequenceCollectionHash g;

	if (!pathIn.empty())
		AssemblyAlgorithms::loadSequences(&g, pathIn.c_str());
	for_each(opt::inFiles.begin(), opt::inFiles.end(), [&g](std::string s) {
		AssemblyAlgorithms::loadSequences(&g, s);
	});
	size_t numLoaded = g.size();
	if (!opt::db.empty())
		addToDb(db, "loadedKmer", numLoaded);
	cout << "Loaded " << numLoaded << " k-mer\n";
	g.setDeletedKey();
	g.shrink();
	if (g.empty()) {
		cerr << "error: no usable sequence\n";
		exit(EXIT_FAILURE);
	}

	AssemblyAlgorithms::setCoverageParameters(AssemblyAlgorithms::coverageHistogram(g));

	if (opt::kc > 0) {
		cout << "Minimum k-mer multiplicity kc is " << opt::kc << endl;
		cout << "Removing low-multiplicity k-mers" << endl;
		size_t removed = AssemblyAlgorithms::applyKmerCoverageThreshold(g, opt::kc);
		cout << "Removed " << removed << " low-multiplicity k-mers, " << g.size()
		     << " k-mers remaining" << std::endl;
	}

	cout << "Generating adjacency" << endl;
	AssemblyAlgorithms::generateAdjacency(&g);

#if PAIRED_DBG
	removePairedDBGInconsistentEdges(g);
#endif

erode:
	if (opt::erode > 0) {
		cout << "Eroding tips" << endl;
		AssemblyAlgorithms::erodeEnds(&g);
		assert(AssemblyAlgorithms::erodeEnds(&g) == 0);
		g.cleanup();
	}

	AssemblyAlgorithms::performTrim(&g);
	g.cleanup();

	if (opt::coverage > 0) {
		removeLowCoverageContigs(g);
		g.wipeFlag(SeqFlag(SF_MARK_SENSE | SF_MARK_ANTISENSE));
		g.cleanup();
		goto erode;
	}

	if (opt::bubbleLen > 0)
		popBubbles(g);

	write_graph(opt::graphPath, g);

	AssemblyAlgorithms::markAmbiguous(&g);
	FastaWriter writer(pathOut.c_str());
	unsigned nContigs = AssemblyAlgorithms::assemble(&g, &writer);
	if (nContigs == 0) {
		cerr << "error: no contigs assembled\n";
		exit(EXIT_FAILURE);
	}

	size_t numAssembled = g.size();
	size_t numRemoved = numLoaded - numAssembled;
	cout << "Removed " << numRemoved
	     << " k-mer.\n"
	        "The signal-to-noise ratio (SNR) is "
	     << 10 * log10((double)numAssembled / numRemoved) << " dB.\n";
}

int
main(int argc, char* const* argv)
{
	Timer timer("Total");

	// Set stdout to be line buffered.
	setvbuf(stdout, NULL, _IOLBF, 0);

#if PAIRED_DBG
	opt::singleKmerSize = -1;
#endif
	opt::parse(argc, argv);

	bool krange = opt::kMin != opt::kMax;
	if (krange)
		cout << "Assembling k=" << opt::kMin << "-" << opt::kMax << ":" << opt::kStep << endl;

	if (!opt::db.empty()) {
		init(
		    db,
		    opt::getUvalue(),
		    opt::getVvalue(),
		    "ABYSS",
		    opt::getCommand(),
		    opt::getMetaValue());
		addToDb(db, "SS", opt::ss);
		addToDb(db, "k", opt::kmerSize);
		addToDb(db, "singleK", opt::singleKmerSize);
		addToDb(db, "numProc", 1);
	}

	for (unsigned k = opt::kMin; k <= opt::kMax; k += opt::kStep) {
		if (krange)
			cout << "Assembling k=" << k << endl;
		opt::kmerSize = k;
#if PAIRED_DBG
		Kmer::setLength(opt::singleKmerSize);
		KmerPair::setLength(opt::kmerSize);
#else
		Kmer::setLength(opt::kmerSize);
#endif

		if (k > opt::kMin) {
			// Reset the assembly options to defaults.
			opt::erode = (unsigned)-1;
			opt::erodeStrand = (unsigned)-1;
			opt::coverage = -1;
			opt::trimLen = k;
			opt::bubbleLen = 3 * k;
		}

		ostringstream k0, k1;
		if (k > opt::kMin)
			k0 << "contigs-k" << k - opt::kStep << ".fa";
		if (k < opt::kMax)
			k1 << "contigs-k" << k << ".fa";
		else
			k1 << opt::contigsPath.c_str();
		assemble(k0.str(), k1.str());
	}

	if (!opt::db.empty())
		addToDb(db, AssemblyAlgorithms::tempStatMap);
	return 0;
}
