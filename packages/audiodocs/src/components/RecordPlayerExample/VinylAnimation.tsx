import React, { useRef, useEffect, useState } from 'react';
import * as THREE from 'three';
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls.js';
import {
  AudioContext,
  AudioBufferSourceNode,
  BiquadFilterNode,
  GainNode,
  AudioBuffer,
} from 'react-native-audio-api';

import labelImage from '../../../static/img/logo.png';
import styles from './styles.module.css';

export default function VinylPlayer(): React.ReactElement {
  const mountRef = useRef<HTMLDivElement>(null);

  const [coverHeight, setCoverHeight] = useState(0);

  const isSliderPressedRef = useRef(false);
  const targetCoverY = useRef(0);
  const recordGroupRef = useRef<THREE.Group | null>(null);
  const coverGroupRef = useRef<THREE.Group | null>(null);
  const stringRef = useRef<THREE.Mesh | null>(null);

  const audioContextRef = useRef<AudioContext | null>(null);
  const bufferSourceRef = useRef<AudioBufferSourceNode | null>(null);
  const audioBufferRef = useRef<AudioBuffer | null>(null);
  const filterRef = useRef<BiquadFilterNode | null>(null);
  const gainRef = useRef<GainNode | null>(null);

  const hitSoundBufferRef = useRef<AudioBuffer | null>(null);
  const wasOnGroundRef = useRef(true);

  const playSound = async () => {
    if (
      !audioContextRef.current ||
      !audioBufferRef.current ||
      bufferSourceRef.current
    ) {
      return;
    }

    const source = await audioContextRef.current.createBufferSource();
    source.buffer = audioBufferRef.current;
    source.loop = true;
    bufferSourceRef.current = source;
    source.connect(filterRef.current!);
    source.start();
  };

  const playHitSound = async () => {
    if (!audioContextRef.current || !hitSoundBufferRef.current) {
      return;
    }
    const source = await audioContextRef.current.createBufferSource();
    source.buffer = hitSoundBufferRef.current;
    source.connect(audioContextRef.current.destination);
    source.start();
  };

  const stopSound = () => {
    if (bufferSourceRef.current) {
      bufferSourceRef.current.stop();
      bufferSourceRef.current = null;
    }
  };

  const handleSliderPress = () => {
    isSliderPressedRef.current = true;
  };

  const handleSliderRelease = () => {
    isSliderPressedRef.current = false;
    targetCoverY.current = 0;
  };

  const handleSliderChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    if (!bufferSourceRef.current) {
      playSound();
    }
    setCoverHeight(Number(e.target.value));
  };

  useEffect(() => {
    const currentMount = mountRef.current;
    if (!currentMount) return;

    const scene = new THREE.Scene();
    scene.background = new THREE.Color(0xe0e0e0);
    const camera = new THREE.PerspectiveCamera(
      75,
      currentMount.clientWidth / currentMount.clientHeight,
      0.1,
      1000
    );
    camera.position.set(8, 10, 12);
    const renderer = new THREE.WebGLRenderer({ antialias: true });
    renderer.setSize(currentMount.clientWidth, currentMount.clientHeight);
    renderer.setPixelRatio(window.devicePixelRatio);
    renderer.shadowMap.enabled = true;
    currentMount.appendChild(renderer.domElement);
    const controls = new OrbitControls(camera, renderer.domElement);
    controls.enableDamping = true;
    controls.target.set(0, 2, 0);
    controls.enabled = false;
    const ambientLight = new THREE.AmbientLight(0xffffff, 0.7);
    scene.add(ambientLight);
    const dirLight = new THREE.DirectionalLight(0xffffff, 1.2);
    dirLight.position.set(10, 15, 5);
    dirLight.castShadow = true;
    dirLight.shadow.camera.top = 10;
    dirLight.shadow.camera.bottom = -10;
    dirLight.shadow.camera.left = -10;
    dirLight.shadow.camera.right = 10;
    dirLight.shadow.bias = 0;
    scene.add(dirLight);

    const bodyMaterial = new THREE.MeshStandardMaterial({
      color: 'rgb(51, 48, 48)',
      metalness: 0,
      roughness: 0.6,
    });
    const boxMaterial = new THREE.MeshStandardMaterial({
      color: '#33488e',
      metalness: 0,
      roughness: 0.7,
    });
    const trimMaterial = new THREE.MeshStandardMaterial({
      color: 'rgb(255, 255, 255)',
      metalness: 0,
      roughness: 0.5,
    });
    const platterMaterial = new THREE.MeshStandardMaterial({
      color: 'rgb(170, 170, 170)',
      metalness: 0,
      roughness: 0.6,
    });
    const recordMaterial = new THREE.MeshStandardMaterial({
      color: 'rgb(26, 26, 26)',
      roughness: 0.6,
      metalness: 0.1,
    });
    const textureLoader = new THREE.TextureLoader();
    const labelTexture = textureLoader.load(labelImage);
    labelTexture.colorSpace = THREE.SRGBColorSpace;
    const invisibleMaterial = new THREE.MeshBasicMaterial({
      transparent: true,
      opacity: 0,
    });
    const labelMaterial = new THREE.MeshStandardMaterial({ map: labelTexture });

    const initAudio = async () => {
      const ctx = new AudioContext();
      audioContextRef.current = ctx;
      const filterNode = ctx.createBiquadFilter();
      filterNode.type = 'lowpass';
      const gainNode = ctx.createGain();
      gainNode.gain.value = 0;
      filterNode.connect(gainNode);
      gainNode.connect(ctx.destination);
      filterRef.current = filterNode;
      gainRef.current = gainNode;
      audioBufferRef.current = await fetch(
        '/react-native-audio-api/audio/music/example-music-01.mp3'
      )
        .then((r) => r.arrayBuffer())
        .then((ab) => ctx.decodeAudioData(ab))
        .catch(() => null);
      hitSoundBufferRef.current = await fetch(
        '/react-native-audio-api/audio/sounds/suitcase-drop.wav'
      )
        .then((r) => r.arrayBuffer())
        .then((ab) => ctx.decodeAudioData(ab))
        .catch(() => null);
    };
    initAudio();

    const ground = new THREE.Mesh(
      new THREE.PlaneGeometry(30, 30),
      new THREE.MeshStandardMaterial({ color: 0xcfcfcf })
    );
    ground.rotation.x = -Math.PI / 2;
    ground.receiveShadow = true;
    scene.add(ground);
    const plinth = new THREE.Mesh(new THREE.BoxGeometry(12, 2, 9), bodyMaterial);
    plinth.castShadow = true;
    plinth.receiveShadow = true;
    scene.add(plinth);
    const platter = new THREE.Mesh(
      new THREE.CylinderGeometry(4, 4, 0.5, 64),
      platterMaterial
    );
    platter.castShadow = true;
    platter.position.y = 1.25;
    scene.add(platter);
    const recordGroup = new THREE.Group();
    recordGroup.position.y = 1.5;
    scene.add(recordGroup);
    recordGroupRef.current = recordGroup;
    const recordBase = new THREE.Mesh(
      new THREE.CylinderGeometry(3.95, 3.95, 0.1, 64),
      recordMaterial
    );
    recordBase.castShadow = true;
    recordGroup.add(recordBase);
    const label = new THREE.Mesh(
      new THREE.CylinderGeometry(1.5, 1.5, 0.11, 64),
      labelMaterial
    );
    label.castShadow = true;
    recordGroup.add(label);
    const NUM_GROOVES = 60;
    const START_RADIUS = 1.7;
    const END_RADIUS = 3.9;
    const GROOVE_THICKNESS = 0.008;
    for (let i = 0; i < NUM_GROOVES; i++) {
      const radius =
        START_RADIUS + (END_RADIUS - START_RADIUS) * (i / (NUM_GROOVES - 1));
      const grooveGeometry = new THREE.TorusGeometry(
        radius,
        GROOVE_THICKNESS,
        8,
        100
      );
      const groove = new THREE.Mesh(grooveGeometry, recordMaterial);
      groove.rotation.x = Math.PI / 2;
      groove.position.y = 0.05;
      recordGroup.add(groove);
    }
    const spindle = new THREE.Mesh(
      new THREE.CylinderGeometry(0.1, 0.1, 0.3, 16),
      platterMaterial
    );
    spindle.castShadow = true;
    spindle.position.y = 0.1;
    recordGroup.add(spindle);
    const tonearmGroup = new THREE.Group();
    tonearmGroup.position.set(4.5, 1, -2.5);
    scene.add(tonearmGroup);
    const tonearmBase = new THREE.Mesh(
      new THREE.CylinderGeometry(0.6, 0.6, 1.2, 32),
      platterMaterial
    );
    tonearmBase.castShadow = true;
    tonearmGroup.add(tonearmBase);
    const arm = new THREE.Mesh(
      new THREE.CylinderGeometry(0.15, 0.15, 7, 32),
      platterMaterial
    );
    arm.castShadow = true;
    arm.rotation.z = Math.PI / 2;
    arm.position.set(-3.5, 0.8, 0);
    tonearmGroup.add(arm);
    const counterweight = new THREE.Mesh(
      new THREE.CylinderGeometry(0.4, 0.4, 1, 32),
      platterMaterial
    );
    counterweight.castShadow = true;
    counterweight.rotation.z = Math.PI / 2;
    counterweight.position.set(0.5, 0.8, 0);
    tonearmGroup.add(counterweight);
    const headshell = new THREE.Mesh(
      new THREE.BoxGeometry(0.4, 0.3, 0.8),
      bodyMaterial
    );
    headshell.castShadow = true;
    headshell.position.set(-6.8, 0.8, 0);
    headshell.rotation.y = -0.15;
    tonearmGroup.add(headshell);
    const button = new THREE.Mesh(
      new THREE.CylinderGeometry(0.5, 0.5, 0.2, 32),
      bodyMaterial
    );
    button.castShadow = true;
    button.position.set(-5, 1.1, 3.5);
    scene.add(button);
    const coverGroup = new THREE.Group();
    scene.add(coverGroup);
    coverGroupRef.current = coverGroup;
    const coverWidth = 13;
    const coverBoxHeight = 6;
    const coverDepth = 10;
    const trimThickness = 0.35;
    const coverGeometry = new THREE.BoxGeometry(
      coverWidth,
      coverBoxHeight,
      coverDepth
    );
    const coverMaterials = [
      boxMaterial,
      boxMaterial,
      boxMaterial,
      invisibleMaterial,
      boxMaterial,
      boxMaterial,
    ];
    const cover = new THREE.Mesh(coverGeometry, coverMaterials);
    cover.scale.set(0.999, 0.999, 0.999);
    cover.castShadow = true;
    cover.position.y = coverBoxHeight / 2;
    coverGroup.add(cover);
    const verticalTrimGeom = new THREE.BoxGeometry(
      trimThickness,
      coverBoxHeight,
      trimThickness
    );
    const cornerX = coverWidth / 2 - trimThickness / 2;
    const cornerZ = coverDepth / 2 - trimThickness / 2;
    const trims = [];
    trims.push(new THREE.Mesh(verticalTrimGeom, trimMaterial));
    trims[0].position.set(cornerX, coverBoxHeight / 2, cornerZ);
    trims.push(new THREE.Mesh(verticalTrimGeom, trimMaterial));
    trims[1].position.set(-cornerX, coverBoxHeight / 2, cornerZ);
    trims.push(new THREE.Mesh(verticalTrimGeom, trimMaterial));
    trims[2].position.set(cornerX, coverBoxHeight / 2, -cornerZ);
    trims.push(new THREE.Mesh(verticalTrimGeom, trimMaterial));
    trims[3].position.set(-cornerX, coverBoxHeight / 2, -cornerZ);
    const fbTrimGeom = new THREE.BoxGeometry(
      coverWidth,
      trimThickness,
      trimThickness
    );
    const lrTrimGeom = new THREE.BoxGeometry(
      trimThickness,
      trimThickness,
      coverDepth - 2 * trimThickness
    );
    const topY = coverBoxHeight - trimThickness / 2;
    const bottomY = trimThickness / 2;
    trims.push(new THREE.Mesh(fbTrimGeom, trimMaterial));
    trims[4].position.set(0, topY, cornerZ);
    trims.push(new THREE.Mesh(fbTrimGeom, trimMaterial));
    trims[5].position.set(0, topY, -cornerZ);
    trims.push(new THREE.Mesh(lrTrimGeom, trimMaterial));
    trims[6].position.set(cornerX, topY, 0);
    trims.push(new THREE.Mesh(lrTrimGeom, trimMaterial));
    trims[7].position.set(-cornerX, topY, 0);
    trims.push(new THREE.Mesh(fbTrimGeom, trimMaterial));
    trims[8].position.set(0, bottomY, cornerZ);
    trims.push(new THREE.Mesh(fbTrimGeom, trimMaterial));
    trims[9].position.set(0, bottomY, -cornerZ);
    trims.push(new THREE.Mesh(lrTrimGeom, trimMaterial));
    trims[10].position.set(cornerX, bottomY, 0);
    trims.push(new THREE.Mesh(lrTrimGeom, trimMaterial));
    trims[11].position.set(-cornerX, bottomY, 0);
    trims.forEach((trim) => {
      trim.castShadow = true;
      coverGroup.add(trim);
    });
    const ANCHOR_Y = 20;
    const COVER_TOP_Y = coverBoxHeight;
    const MAX_STRING_LENGTH = ANCHOR_Y - COVER_TOP_Y;
    const stringGeometry = new THREE.CylinderGeometry(
      0.1,
      0.1,
      MAX_STRING_LENGTH,
      8
    );
    const string = new THREE.Mesh(stringGeometry, bodyMaterial);
    stringRef.current = string;
    coverGroup.add(string);

    const animate = () => {
      requestAnimationFrame(animate);

      if (recordGroupRef.current) {
        recordGroupRef.current.rotation.y += 0.1;
      }

      if (coverGroupRef.current && stringRef.current) {
        const currentY = coverGroupRef.current.position.y;
        const LERP_FACTOR_ACTIVE = 0.1;
        const LERP_FACTOR_FALLING = 0.04;
        const lerpFactor = isSliderPressedRef.current
          ? LERP_FACTOR_ACTIVE
          : LERP_FACTOR_FALLING;
        coverGroupRef.current.position.y +=
          (targetCoverY.current - currentY) * lerpFactor;

        const newCoverY = coverGroupRef.current.position.y;

        if (targetCoverY.current === 0 && newCoverY < 0.09) {
          coverGroupRef.current.position.y = 0;
}

        if (!isSliderPressedRef.current) {
          const maxCoverHeight3D = 7; 
          const maxSliderValue = 10; 
          const newSliderValue = (newCoverY / maxCoverHeight3D) * maxSliderValue;
          setCoverHeight(Math.max(0, newSliderValue)); 
        }

        if (filterRef.current && gainRef.current && audioContextRef.current) {
          const maxCoverHeight3D = 7;
          const clampedY = Math.max(0, Math.min(newCoverY, maxCoverHeight3D));
          const ratio = clampedY / maxCoverHeight3D;
          const currentTime = audioContextRef.current.currentTime;
          filterRef.current.frequency.setTargetAtTime(
            200 + ratio * (8000 - 200),
            currentTime,
            0.015
          );
          gainRef.current.gain.setTargetAtTime(
            ratio * 0.5,
            currentTime,
            0.015
          );
        }

        const isOnGround = newCoverY < 0.01;
        if (isOnGround && !wasOnGroundRef.current) {
          playHitSound();
          if (bufferSourceRef.current) {
            stopSound();
          }
        }
        wasOnGroundRef.current = isOnGround;

        const newStringLength = ANCHOR_Y - (newCoverY + COVER_TOP_Y);
        stringRef.current.scale.y = newStringLength / MAX_STRING_LENGTH;
        stringRef.current.position.y = COVER_TOP_Y + newStringLength / 2;
      }

      controls.update();
      renderer.render(scene, camera);
    };
    animate();

    const handleResize = () => {
      if (!currentMount) return;
      camera.aspect = currentMount.clientWidth / currentMount.clientHeight;
      camera.updateProjectionMatrix();
      renderer.setSize(currentMount.clientWidth, currentMount.clientHeight);
    };
    window.addEventListener('resize', handleResize);

    return () => {
      window.removeEventListener('resize', handleResize);
      scene.traverse((object) => {
        if (object instanceof THREE.Mesh) {
          if (object.geometry) object.geometry.dispose();
          if (Array.isArray(object.material)) {
            object.material.forEach((material) => {
              if (material.map) material.map.dispose();
              material.dispose();
            });
          } else if (object.material) {
            if (object.material.map) object.material.map.dispose();
            object.material.dispose();
          }
        }
      });
      labelTexture.dispose();
      renderer.dispose();
      if (currentMount && currentMount.contains(renderer.domElement)) {
        currentMount.removeChild(renderer.domElement);
      }
      audioContextRef.current?.close();
    };
  }, []);

  useEffect(() => {
    // Only update the target if the user is actively dragging the slider.
    if (isSliderPressedRef.current) {
      const maxCoverHeight3D = 7;
      const maxSliderValue = 10;
      targetCoverY.current = (coverHeight / maxSliderValue) * maxCoverHeight3D;
    }
  }, [coverHeight]);

  return (
    <div
      style={{
        width: '100%',
        display: 'flex',
        flexDirection: 'column',
        alignItems: 'center',
      }}
    >
      <div
        ref={mountRef}
        style={{
          width: '100%',
          height: '600px',
          background: '#e0e0e0',
        }}
      />
      <div
        style={{
          padding: '16px',
          background: '#f0f0f0',
          borderRadius: '8px',
          marginTop: '16px',
          display: 'flex',
          alignItems: 'center',
          gap: '10px',
        }}
      >
        <span>Cover Down</span>
        <input
          type="range"
          min="0"
          max="10"
          step="0.1"
          value={coverHeight}
          onChange={handleSliderChange}
          onMouseDown={handleSliderPress}
          onMouseUp={handleSliderRelease}
          onTouchStart={handleSliderPress}
          onTouchEnd={handleSliderRelease}
          style={{ width: '200px' }}
        />
        <span>Cover Up</span>
      </div>
    </div>
  );
}