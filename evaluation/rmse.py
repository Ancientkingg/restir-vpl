import numpy as np
import imageio.v3 as iio

# This script computes the RMSE of different sampling techniques over a series of frames
# and plots the results.

# Number of frames to process
framecount_folder = 500
framecount = 20

# Define the source folder containing the images
source_folder = "../images/cornell_box"

# Define the prefixes for each sampling technique
uniform_prefix = "Uniform_frame"
ris_prefix = "RIS_frame"
restir_prefix = "ReSTIR_frame"

# Define the folders for each sampling technique
uniform_sampling_folder = f"{source_folder}/uniform_{framecount_folder}"
ris_sampling_folder = f"{source_folder}/ris_{framecount_folder}"
restir_sampling_folder = f"{source_folder}/restir_{framecount_folder}"

# Load PFM
ground_truth_img = iio.imread(f"{source_folder}/reference.pfm") / 255

# Existing buffers to accumulate frames over
uniform_accumulated = np.zeros_like(ground_truth_img, dtype=np.float32)
ris_accumulated = np.zeros_like(ground_truth_img, dtype=np.float32)
restir_accumulated = np.zeros_like(ground_truth_img, dtype=np.float32)

# Accumulate values into buffer
def accumulate_frame(buffer, img, frame_number):
	buffer = (buffer * frame_number + img) / (frame_number + 1);
	return buffer

uniform_rmse_values = []
ris_rmse_values = []
restir_rmse_values = []

# For loop over framecount
for i in range(framecount):
	print(f"\rProcessing frame {i + 1}/{framecount}", end='')

	# Load the images for each sampling technique
	uniform_img = iio.imread(f"{uniform_sampling_folder}/{uniform_prefix}{i:04d}.pfm") / 255
	ris_img = iio.imread(f"{ris_sampling_folder}/{ris_prefix}{i:04d}.pfm") / 255
	restir_img = iio.imread(f"{restir_sampling_folder}/{restir_prefix}{i:04d}.pfm") / 255

	# Accumulate into buffers
	uniform_accumulated = accumulate_frame(uniform_accumulated, uniform_img, i)
	ris_accumulated = accumulate_frame(ris_accumulated, ris_img, i)
	restir_accumulated = accumulate_frame(restir_accumulated, restir_img, i)

	# Compute RMSE compared to ground truth
	uniform_rmse = np.sqrt(np.mean((uniform_accumulated - ground_truth_img) ** 2))
	ris_rmse = np.sqrt(np.mean((ris_accumulated - ground_truth_img) ** 2))
	restir_rmse = np.sqrt(np.mean((restir_accumulated - ground_truth_img) ** 2))

	# Store the RMSE values
	uniform_rmse_values.append(uniform_rmse)
	ris_rmse_values.append(ris_rmse)
	restir_rmse_values.append(restir_rmse)

# Save final buffers as images
# iio.imwrite(f"{uniform_sampling_folder}/manual_accumulate_Uniform_frame{framecount:04d}.pfm", uniform_accumulated)
# iio.imwrite(f"{ris_sampling_folder}/manual_accumulate_RIS_frame{framecount:04d}.pfm", ris_accumulated)
# iio.imwrite(f"{restir_sampling_folder}/manual_accumulate_ReSTIR_frame{framecount:04d}.pfm", restir_accumulated)

# Plot the RMSE values against the frame number
import matplotlib.pyplot as plt
import seaborn as sns

sns.set(style='whitegrid')

plt.figure(figsize=(8, 8))

# Plot the RMSE values
plt.plot(range(framecount), uniform_rmse_values, label='Uniform', color='tab:blue', linewidth=2)
plt.plot(range(framecount), ris_rmse_values, label='RIS', color='tab:green', linewidth=2)
plt.plot(range(framecount), restir_rmse_values, label='ReSTIR', color='tab:red', linewidth=2)

# Set axis labels and title
plt.xlabel('Frame Number', fontsize=20)
plt.ylabel('RMSE', fontsize=20)
plt.title('Sahur', fontsize=22)

# Set axis font size
plt.xticks(fontsize=16)
plt.yticks(fontsize=16)

# Set custom x-ticks
plt.xticks([1, 2, 5, 10, 20])

# Set axis limits (start from 0)
plt.xlim(left=0)
plt.ylim(bottom=0)


# Grid and legend
plt.grid(True, linestyle='--', linewidth=0.5)

# Put legend outside below the plot
plt.legend(loc='upper center', bbox_to_anchor=(0.5, -0.1),
           ncol=3, fontsize=16, frameon=False)

# Adjust layout to make room for the legend
plt.tight_layout(rect=[0, 0.05, 1, 1])  # leave space at bottom

# Save and show the plot
plt.savefig('rmse_comparison.png', dpi=300, bbox_inches='tight')
plt.show()


# Save RMSE values to a text file
with open('rmse_values.txt', 'w') as f:
	f.write("Frame Number,Uniform RMSE,RIS RMSE,ReSTIR RMSE\n")
	for i in range(framecount):
		f.write(f"{i},{uniform_rmse_values[i]},{ris_rmse_values[i]},{restir_rmse_values[i]}\n")
