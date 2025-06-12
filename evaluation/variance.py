import numpy as np
import imageio.v3 as iio  # Requires imageio >=2.16

# Load PFM
image = iio.imread("accumulate_Uniform_frame0005.pfm")

# Convert to luminance
luminance = 0.2126 * image[..., 0] + 0.7152 * image[..., 1] + 0.0722 * image[..., 2]

# Flatten and compute variance
luminance_flat = luminance.flatten()
variance = np.var(luminance_flat)

print("Variance:", variance)