//
//  camera.h
//  Automata
//
//  Created by Antoni Wójcik on 07/05/2020.
//  Copyright © 2020 Antoni Wójcik. All rights reserved.
//

#ifndef camera_h
#define camera_h

#include "shader.h"

#define ZOOM_MIN 1.0f
#define ZOOM_MAX 0.05f
#define MOVE_SPEED 0.002f
#define ZOOM_SPEED 0.1f

class Camera {
private:
    float tex_width_n, tex_height_n; // normalized texture coords
    float tex_aspect;
    
    float scr_width, scr_height, scr_max;
    float width, height;
    float pos_x, pos_y;
    float zoom_current;
    
    void reposition() {
        if(pos_x < -scr_width * 0.5f) pos_x += scr_width;
        else if(pos_x > scr_width * 0.5f) pos_x -= scr_width;
        
        if(pos_y < -scr_height * 0.5f) pos_y += scr_height;
        else if(pos_y > scr_height * 0.5f) pos_y -= scr_height;
    }
    
    void setTextureSize() {
        if((scr_height / scr_width) >= tex_aspect) {
            tex_width_n = 1.0f;
            tex_height_n = scr_width / scr_height * tex_aspect;
        } else {
            tex_width_n = scr_height / (scr_width * tex_aspect);
            tex_height_n = 1.0f;
        }
    }

public:
    Camera(unsigned int scr_width_u, unsigned int scr_height_u, int tex_width_u, int tex_height_u) {
        tex_aspect = (float)tex_height_u / (float)tex_width_u;
        
        resize(scr_width_u, scr_height_u);
        
        pos_x = 0.0f;
        pos_y = 0.0f;
    }
    
    void zoom(float offset) {
        zoom_current *= exp(-offset * ZOOM_SPEED);
        
        if(zoom_current > ZOOM_MIN) zoom_current = ZOOM_MIN;
        else if(zoom_current < ZOOM_MAX) zoom_current = ZOOM_MAX;
        
        width = scr_width * zoom_current;
        height = scr_height * zoom_current;
        
        //reposition();
    }
    
    void move(float offset_x, float offset_y) {
        pos_x += offset_x * zoom_current * scr_max * MOVE_SPEED;
        pos_y += offset_y * zoom_current * scr_max * MOVE_SPEED;
        
        //reposition();
    }
    
    void resize(unsigned int scr_width_u, unsigned int scr_height_u) {
        scr_width = (float)(scr_width_u) * 0.5f;
        scr_height = (float)(scr_height_u) * 0.5f;
        
        scr_max = (scr_width >= scr_height) ? scr_width : scr_height;
        
        width = scr_width;
        height = scr_height;
        zoom_current = 1.0f;
        
        setTextureSize();
    }
    
    void transferData(Shader& shader, const std::string& pos_x_id, const std::string& pos_y_id, const std::string& width_id, const std::string& height_id) const {
        shader.use();
        
        shader.setFloat(pos_x_id, pos_x / scr_max);
        shader.setFloat(pos_y_id, pos_y / scr_max);
        shader.setFloat(width_id, zoom_current / tex_width_n);
        shader.setFloat(height_id, zoom_current / tex_height_n);
    }
};

#endif /* camera_h */
