# RayFinder

In recent years, indoor gardening has gained significant attention due to its numerous benefits, including improving air quality, reducing stress levels, and enhancing the aesthetic appeal of living spaces. However, selecting and positioning plants in a room can be a daunting task, as their survival depends on several factors, including light exposure.  
  
  
![Untitled-1](https://user-images.githubusercontent.com/56971054/231934703-03c63698-4430-469f-b8e8-4b84411fbce2.gif)

  

RayFinder was built with the goal of offering a visual representation of how light hits a plant depending on the orientation of the window in order to maximise successful plant growth. Plants can be extremely particular... 😅 and they need to be placed either under full sun, part sun or shade. Since we are busy people who can't sit in front of their window all day to determine the kind of light it receives, RayFinder does it all for you in just a few seconds!

## Functionality 
<img width="192" alt="image" src="https://user-images.githubusercontent.com/56971054/231935438-7b4ae52d-df95-403f-9962-aae4676b54bb.png">
This OpenGL project relies on the ImGUI library to let users select a personalized window orientation. The above menu can generate scenes based on latitude, longitude, day of the year, as well as the physical orientation of the window. Users can also view the window at a fixed time of the day. Running the scene then generates a virtual room that loops through the selected day with realistic light animation. 

## How it Works

<img width="599" alt="img1" src="https://user-images.githubusercontent.com/56971054/231938060-63882255-c0fa-40b1-a941-22565e718f2b.png">
  
### The Light
The scene presents a room with a table and a plant. The models are parsed for OpenGL using the Assimp library and the textures. The sun is simulated through direct lighting aimed at the window and its movement is determined by the [Solar Position Algorithm](https://midcdmz.nrel.gov/spa/). The sky's color dynamically changes when the sun crosses the horizon, and the color is smoothly interpolated between two targets based on the time of day, providing a more realistic and immersive experience.

### The Shadows
To convey a certain sense of realism, it was vital that the light also cast shadows, especially the room itself. Through shadow mapping, the plant and table cast shadows on the walls and vice-versa. A depth map is initially created to calculate the various distance from the light source to the objects in the scene. The scene is then rendered again from the point of view of the camera. While rendering, the distance from the light of each point in the scene is compared to the values stored in the depth map. If the distance is greater than the value in the depth map, that point is in shadow.  

## Implementation
**API:** `OpenGL in C++`  
**Dependencies:** `GLFW`, `GLAD`, `GLM`, `ImGUI`, `Assimp`, `STBimage`


  
