import cv2
import numpy as np
import os
import glob

sample_dir = r"c:\AAAAAAAAAAA_temp\desktop\Repository\App\Sudoku_demo\Sudoku_Sample"
files = glob.glob(os.path.join(sample_dir, "*.png"))

for f in files[:1]:
    print("Processing:", f)
    src = cv2.imread(f)
    if src is None:
        print("Failed to read")
        continue
    print("Shape:", src.shape)
    
    # 1a
    hsv = cv2.cvtColor(src, cv2.COLOR_BGR2HSV)
    blueMask = cv2.inRange(hsv, np.array([85, 20, 80]), np.array([135, 255, 255]))
    
    # 1b
    gray = cv2.cvtColor(src, cv2.COLOR_BGR2GRAY)
    gray_blur = cv2.GaussianBlur(gray, (3, 3), 0)
    adaptBin = cv2.adaptiveThreshold(gray_blur, 255, cv2.ADAPTIVE_THRESH_GAUSSIAN_C, cv2.THRESH_BINARY_INV, 11, 2)
    
    # 1c
    combined = cv2.bitwise_or(blueMask, adaptBin)
    
    # 1d
    kernelH = cv2.getStructuringElement(cv2.MORPH_RECT, (5, 1))
    kernelV = cv2.getStructuringElement(cv2.MORPH_RECT, (1, 5))
    dilH = cv2.dilate(combined, kernelH)
    dilV = cv2.dilate(combined, kernelV)
    combined = cv2.bitwise_or(dilH, dilV)
    
    # 2
    minVotes = int(min(combined.shape[0], combined.shape[1]) * 0.10)
    minVotes = max(minVotes, 20)
    lines = cv2.HoughLines(combined, 1, np.pi / 180, minVotes)
    
    rawH = []
    rawV = []
    if lines is not None:
        for l in lines:
            rho, theta = l[0]
            if theta < np.pi / 12 or theta > np.pi * 11 / 12:
                rawV.append(rho)
            elif abs(theta - np.pi / 2) < np.pi / 12:
                rawH.append(rho)
                
    print("rawH count:", len(rawH))
    print("rawV count:", len(rawV))
    
    cv2.imwrite("test_blue.png", blueMask)
    cv2.imwrite("test_adapt.png", adaptBin)
    cv2.imwrite("test_combined.png", combined)
