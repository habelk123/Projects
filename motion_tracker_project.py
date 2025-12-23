import cv2

cap = cv2.VideoCapture(0)

#Kernel for Opening/closing morphology
kernel = cv2.getStructuringElement(cv2.MORPH_RECT, (3,3))

#Capture first frame to store for comparison in loop
ret_first, first_frame = cap.read()
if not ret_first:
        print("Failed to grab frame")

gray_first_frame = cv2.cvtColor(first_frame, cv2.COLOR_BGR2GRAY)
blur_first_frame = cv2.GaussianBlur(gray_first_frame, (5,5), 0)
prev_frame = blur_first_frame

#initializing centroid variables
prev_centroid = None
no_motion_frames = 0 #tracks the number of frames with no motion present

#initialize overlay parameters
status = ""
color = (0,0,0)
#Loop for frame processing/comparison
while True:
    ret, curr_frame = cap.read()
    if not ret:
        print("Failed to grab frame")
        break
    #Display livestream frame by frame
    cv2.imshow("Webcam", curr_frame)

    #Preprocess current frame (grayscale + blur)
    gray_frame = cv2.cvtColor(curr_frame, cv2.COLOR_BGR2GRAY)
    blur_frame = cv2.GaussianBlur(gray_frame, (5,5), 0)

    #Frame differencing
    frame_diff = cv2.absdiff(blur_frame, prev_frame)
    

    #Thresholding - Pixel Intensity (quantifies what valid motion is)
    thresh_value, thresh_img = cv2.threshold(frame_diff, 10, 255, cv2.THRESH_BINARY)
    

    #Applying Morphology - Opening --> Closing, to clean-up noise artifacts and join dislocated regions
    opened_img = cv2.morphologyEx(thresh_img, cv2.MORPH_OPEN, kernel)
    closed_img = cv2.morphologyEx(opened_img, cv2.MORPH_CLOSE, kernel)
    

    #Contouring: identify regions of motion detection (pixel's on vs pixel's off)
    contours, hierarchy = cv2.findContours(
        closed_img,
        cv2.RETR_EXTERNAL,
        cv2.CHAIN_APPROX_SIMPLE
    )
    
    #Filtering Contours based on area size threshold 
    filtered_contours = []
    for cnt in contours:
         if cv2.contourArea(cnt) > 100:
              filtered_contours.append(cnt)

    #Processing Filtered Contours for Motion Detection/Tracking 

    #Resetting Centroid tracking when motion is no longer detected after 15 frames
    if not filtered_contours:
        no_motion_frames += 1
        if no_motion_frames >= 15:
            prev_centroid = None
            status = "IDLE" #overlay status when no motion detected
            color = (0,0,255)
    
    elif filtered_contours:
        no_motion_frames = 0
        #Obtain the largest contour region (most significant motion)
        largest_contour = max(filtered_contours, key = cv2.contourArea)

        #Centroid Calculation for Motion tracking
        contour_moments = cv2.moments(largest_contour) #dictionary of frame moments
        if contour_moments["m00"] != 0:
            cx = int(contour_moments["m10"] / contour_moments["m00"]) #centroid x-coordinate
            cy = int(contour_moments["m01"] / contour_moments["m00"]) #centroid y-coordinate
            curr_centroid = (cx, cy) #store as a point
            cv2.circle(curr_frame, curr_centroid, 4, (0,0,255), -1) #draw centroid
            if prev_centroid is None:
                prev_centroid = (cx, cy)
            else:
                cv2.line(curr_frame, prev_centroid, curr_centroid, (255,0,0), 2) #draw trail from previous centroid to current
                status = "TRACKING" #overlay status when motion detected
                color = (0,255,0)
                prev_centroid = curr_centroid
             
        #Applying Bounding Box's
        x_bb,y_bb, w_bb, h_bb = cv2.boundingRect(largest_contour)
        cv2.rectangle(curr_frame, (x_bb, y_bb), 
                       (x_bb + w_bb, y_bb + h_bb), (0,255,0), 2)

    #Display livestream with motion tracking applied
    cv2.putText(curr_frame, status, (10,30), 2, 0.7, color, 2) #overlay identifying state of tracker
    cv2.putText(curr_frame, "By: Habel", (500,30), 2, 0.7, (255,0,0), 2)
    cv2.imshow("Webcam - Motion Tracking", curr_frame)

    prev_frame = blur_frame #store current frame for next iteration

    if cv2.waitKey(1) & 0xFF == ord('q'): #close livestream window(s)
        break

cap.release()
cv2.destroyAllWindows()
