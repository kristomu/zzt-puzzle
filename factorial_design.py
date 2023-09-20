# Given a number of output logs from zzt-puzzle,
# create a factorial experimental design to can be used to
# fit an estimated difficulty function later.

# Maybe I'll do Latin square or something else later?

import numpy as np
import itertools

def read_puzzle_metadata(puzzle_file, puzzle_data={}):

	for line in puzzle_file:

		# We're only interested in the metadata.
		if not "summary:" in line:
			continue

		metadata_words = line.split()

		name = metadata_words[1].rstrip(":")
		values = [float(x) for x in metadata_words[3:]]

		puzzle_data[name] = values

	return puzzle_data

# Return the row indices of the k rows in the haystack with
# the best Euclidean distance to the needle.
def k_best_euclidean(haystack, needle, k):
	distances = np.linalg.norm(haystack - needle, axis=1)
	return np.argsort(distances)[:k]

# Read the files.
# You may want to add different file names here.
# These files are logs obtained by ./zzt-scan |tee filename.txt
y = {}
for file in ["para_skip_test_III.txt", "para_skip_test_II.txt",
	"para_skip_test_IV.txt", "para_skip_test.txt", "para_skip_test_VII.txt",
	"para_skip_test_VI.txt", "para_skip_test_V.txt"]:
	y = read_puzzle_metadata(open(file, "r"), y)
	print("Read %d puzzles" % len(y))

# Get the keys and values
y_keys = tuple(y.keys())
y_values = np.array(list(y.values()))
# Remove the column with index 11 as that's the raw nodes
# visited count, which I'm pretty sure isn't important.
reduced_y_values = np.delete(y_values, 11, axis=1)
# Also remove axis 1 

# Get the percentiles that will stand in for each factor level.
levels = 2
percentiles = np.percentile(reduced_y_values, axis=0,
	q=np.linspace(0, 100, levels))

# Get the possible combinations of factor levels that we can
# use. The use of percentiles work to standardize things here,
# although I'm not sure if it generalizes to more than two
# levels. If we have three, we should probably use min, (min+max)/2,
# max, for isntance, even if the distribution is very skewed.
products = np.array(list(itertools.product(*percentiles.T)))

# How many puzzles (examples) from each experiment do we want?
puzzles_per_experiment = 10
puzzle_indices_chosen = set()
for i in range(len(products)):
	print("Progress: %.5f, puzzles: %d" % (i/len(products),
		len(puzzle_indices_chosen)))
	best_indices = k_best_euclidean(reduced_y_values, products[i],
		puzzles_per_experiment)
	puzzle_indices_chosen = puzzle_indices_chosen.union(best_indices)

puzzles = [y_keys[i] for i in puzzle_indices_chosen]
print(puzzles)
# And now you can do something like say
# open("factorial_design_10.txt", "w"")