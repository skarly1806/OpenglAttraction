#pragma once

#include <vector>
#include "common.hpp"
#include "glm.hpp"
#include "glm.hpp"

namespace glimac {

// Représente une camera qui cible un point précis

    class TrackballCamera {
    public:
        // Constructeur: alloue le tableau de données et construit les attributs des vertex
        TrackballCamera()
        {
            this->m_fAngleX = 0.f;
            this->m_fAngleY = 0.f;
            this->m_fDistance = 5.f;
        }

        float getAngleX()
        {
            return m_fAngleX;
        }

        float getAngleY()
        {
            return m_fAngleY;
        }

        float getDistance()
        {
            return m_fDistance;
        }

        void setDistance(float delta){
            m_fDistance = delta;
        }

        /// @brief Permet d'avancer / reculer la caméra de la distance delta (delta positif avance la camera)
        /// @param delta
        void moveFront(float delta)
        {
            m_fDistance -= delta;
        }

        /// @brief Permet de tourner horizontalement autour du centre de vision
        /// @param degrees
        void rotateLeft(float degrees)
        {
            m_fAngleX = glm::radians(degrees);
        }

        /// @brief Permet de tourner verticalement autour du centre de vision
        /// @param degrees
        void rotateUp(float degrees)
        {
            m_fAngleY = glm::radians(degrees);
        }

        glm::mat4 getViewMatrix() const
        {
            glm::mat4 ViewMatrix = glm::mat4(1);
            glm::mat4 MoveFrontMatrix = glm::translate(ViewMatrix, glm::vec3(0, 0, -m_fDistance));
            glm::mat4 RotateLeftMatrix = glm::rotate(ViewMatrix, m_fAngleX, glm::vec3(1, 0, 0));
            glm::mat4 RotateUpMatrix   = glm::rotate(ViewMatrix, m_fAngleY, glm::vec3(0, 1, 0));
            return MoveFrontMatrix * RotateLeftMatrix * RotateUpMatrix;
        }

    private:
        float m_fDistance; // distance au centre
        float m_fAngleX; // angle autour de l'axe x (haut et bas)
        float m_fAngleY; // angle autour de l'axe y (gauche et droite)
    };

} // namespace glimac