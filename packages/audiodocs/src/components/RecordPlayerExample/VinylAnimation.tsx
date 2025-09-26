import React, { useRef, useEffect, useMemo, useState } from 'react';
import * as THREE from 'three';
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls.js';
import { RoundedBoxGeometry } from 'three/examples/jsm/geometries/RoundedBoxGeometry.js';
import { LineSegments2 } from 'three/examples/jsm/lines/LineSegments2.js';
import { LineSegmentsGeometry } from 'three/examples/jsm/lines/LineSegmentsGeometry.js';
import { LineMaterial } from 'three/examples/jsm/lines/LineMaterial.js';

// @ts-ignore
import labelImage from '/static/img/logo.png';
// @ts-ignore
import swmLogo from '/static/img/swm-text.png';
import styles from './styles.module.css';
import { VINYL_CONSTANTS as C } from './consts';

interface SceneRefs {
  recordGroup: THREE.Group;
  coverGroup: THREE.Group;
  string: THREE.Mesh;
}

interface AnimationState {
  isSliderPressed: boolean;
  targetCoverY: number;
  coverHeight: number;
  wasOnGround: boolean;
  fallVelocity: number;
}

export default function VinylPlayer(): React.ReactElement {
  const mountRef = useRef<HTMLDivElement>(null);
  const inputRef = useRef<HTMLInputElement>(null);

  // Animation state ref 
  const animationStateRef = useRef<AnimationState>({
    isSliderPressed: false,
    targetCoverY: 0,
    coverHeight: 0,
    wasOnGround: true,
    fallVelocity: 0,
  });

  // Scene object refs 
  const sceneRefsRef = useRef<SceneRefs | null>(null);

  const { context, filter, gain } = useMemo(() => {
    const ctx = new AudioContext();
    const filterNode = ctx.createBiquadFilter();
    filterNode.type = 'lowpass';
    const gainNode = ctx.createGain();
    gainNode.gain.value = 0;
    filterNode.connect(gainNode);
    gainNode.connect(ctx.destination);
    return { context: ctx, filter: filterNode, gain: gainNode };
  }, []);

  const [audioBuffer, setAudioBuffer] = useState<AudioBuffer | null>(null);

  const bufferSourceRef = useRef<AudioBufferSourceNode | null>(null);

  useEffect(() => {
    const fetchAudio = async () => {
      try {
        const buffer = await fetch('/react-native-audio-api/audio/music/pass-the-mayo.mp3')
          .then((r) => r.arrayBuffer())
          .then((ab) => context.decodeAudioData(ab));
        setAudioBuffer(buffer);
      } catch (error) {
        console.warn('Failed to load audio:', error);
      }
    };

    fetchAudio();

    return () => {
      context?.close();
    };
  }, [context]); 

  const playSound = () => {
    if (!context || !audioBuffer || bufferSourceRef.current) {
      return;
    }
    const source = context.createBufferSource();
    source.buffer = audioBuffer;
    source.loop = true;
    source.connect(filter);
    source.start();
    bufferSourceRef.current = source;
  };

  const stopSound = () => {
    if (bufferSourceRef.current) {
      bufferSourceRef.current.stop();
      bufferSourceRef.current = null;
    }
  };

  const handleSliderPress = () => {
    animationStateRef.current.isSliderPressed = true;
  };

  const handleSliderRelease = () => {
    animationStateRef.current.isSliderPressed = false;
    animationStateRef.current.targetCoverY = 0;
  };

  const handleSliderChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const v = Number(e.target.value);
    const state = animationStateRef.current;
    state.coverHeight = v;
    state.targetCoverY = (v / C.SLIDER_MAX) * C.SLIDER_MAX;
    
    if (!bufferSourceRef.current) {
      playSound();
    }
  };

  const initializeScene = (container: HTMLDivElement) => {
    const scene = new THREE.Scene();
    const camera = new THREE.PerspectiveCamera(
      75,
      container.clientWidth / container.clientHeight,
      0.1,
      1000
    );
    camera.position.set(8, 10, 12);
    
    const renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
    renderer.setSize(container.clientWidth, container.clientHeight);
    renderer.setPixelRatio(window.devicePixelRatio);
    renderer.shadowMap.enabled = true;
    container.appendChild(renderer.domElement);
    
    const controls = new OrbitControls(camera, renderer.domElement);
    controls.enableDamping = true;
    controls.target.set(0, 2, 0);
    controls.enabled = false;

    return { scene, camera, renderer, controls };
  };

  const initializeLighting = (scene: THREE.Scene) => {
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
  };

  const createMaterials = () => {
    const textureLoader = new THREE.TextureLoader();
    const labelTexture = textureLoader.load(labelImage);
    labelTexture.colorSpace = THREE.SRGBColorSpace;

    return {
      body: new THREE.MeshStandardMaterial({
        color: 'rgb(51, 48, 48)',
        metalness: 0,
        roughness: 0.6,
      }),
      box: new THREE.MeshStandardMaterial({
        color: '#0a2688',
        metalness: 0,
        roughness: 0.7,
      }),
      platter: new THREE.MeshStandardMaterial({
        color: 'rgb(170, 170, 170)',
        metalness: 0,
        roughness: 0.6,
      }),
      record: new THREE.MeshStandardMaterial({
        color: 'rgb(26, 26, 26)',
        roughness: 0.6,
        metalness: 0.1,
      }),
      invisible: new THREE.MeshBasicMaterial({
        transparent: true,
        opacity: 0,
      }),
      label: new THREE.MeshStandardMaterial({ map: labelTexture }),
      labelTexture,
    };
  };

  const createVinylPlayer = (scene: THREE.Scene, materials: ReturnType<typeof createMaterials>) => {
    // Main body
    const plinth = new THREE.Mesh(
      new RoundedBoxGeometry(12, 2, 9, 4, 0.2),
      materials.body
    );
    plinth.castShadow = true;
    plinth.receiveShadow = true;
    plinth.position.y = 1;
    scene.add(plinth);

    // Platter
    const platter = new THREE.Mesh(
      new THREE.CylinderGeometry(4, 4, 0.5, 64),
      materials.platter
    );
    platter.castShadow = true;
    platter.position.y = 2.25;
    scene.add(platter);

    // Record group
    const recordGroup = new THREE.Group();
    recordGroup.position.y = 2.55;
    scene.add(recordGroup);

    const recordBase = new THREE.Mesh(
      new THREE.CylinderGeometry(3.95, 3.95, 0.1, 64),
      materials.record
    );
    recordBase.castShadow = true;
    recordGroup.add(recordBase);

    const label = new THREE.Mesh(
      new THREE.CylinderGeometry(1.5, 1.5, 0.11, 64),
      materials.label
    );
    label.castShadow = true;
    recordGroup.add(label);

    // Grooves
    const NUM_GROOVES = 60;
    const START_RADIUS = 1.7;
    const END_RADIUS = 3.9;
    const GROOVE_THICKNESS = 0.008;
    for (let i = 0; i < NUM_GROOVES; i++) {
      const radius = START_RADIUS + (END_RADIUS - START_RADIUS) * (i / (NUM_GROOVES - 1));
      const grooveGeometry = new THREE.TorusGeometry(radius, GROOVE_THICKNESS, 8, 100);
      const groove = new THREE.Mesh(grooveGeometry, materials.record);
      groove.rotation.x = Math.PI / 2;
      groove.position.y = 0.05;
      recordGroup.add(groove);
    }

    const spindle = new THREE.Mesh(
      new THREE.CylinderGeometry(0.1, 0.1, 0.3, 16),
      materials.platter
    );
    spindle.castShadow = true;
    spindle.position.y = 0.1;
    recordGroup.add(spindle);

    // Tonearm group
    const tonearmGroup = new THREE.Group();
    tonearmGroup.position.set(4.5, 2, -2.5);
    scene.add(tonearmGroup);

    const tonearmBase = new THREE.Mesh(
      new THREE.CylinderGeometry(0.6, 0.6, 1.2, 32),
      materials.platter
    );
    tonearmBase.castShadow = true;
    tonearmGroup.add(tonearmBase);

    const arm = new THREE.Mesh(
      new THREE.CylinderGeometry(0.15, 0.15, 7, 32),
      materials.platter
    );
    arm.castShadow = true;
    arm.rotation.z = Math.PI / 2;
    arm.position.set(-3.5, 0.8, 0);
    tonearmGroup.add(arm);

    const counterweight = new THREE.Mesh(
      new THREE.CylinderGeometry(0.4, 0.4, 1, 32),
      materials.platter
    );
    counterweight.castShadow = true;
    counterweight.rotation.z = Math.PI / 2;
    counterweight.position.set(0.5, 0.8, 0);
    tonearmGroup.add(counterweight);

    const headshell = new THREE.Mesh(
      new RoundedBoxGeometry(0.4, 0.3, 0.8, 2, 0.05),
      materials.body
    );
    headshell.castShadow = true;
    headshell.position.set(-6.8, 0.8, 0);
    headshell.rotation.y = -0.15;
    tonearmGroup.add(headshell);

    // Button
    const button = new THREE.Mesh(
      new THREE.CylinderGeometry(0.5, 0.5, 0.2, 32),
      materials.body
    );
    button.castShadow = true;
    button.position.set(-5, 2.1, 3.5);
    scene.add(button);

    return recordGroup;
  };

  const createCoverBox = (scene: THREE.Scene, renderer: THREE.WebGLRenderer, materials: ReturnType<typeof createMaterials>) => {
    const coverGroup = new THREE.Group();
    scene.add(coverGroup);

    const coverWidth = 13;
    const coverBoxHeight = 6;
    const coverDepth = 10;

    const coverGeometry = new RoundedBoxGeometry(coverWidth, coverBoxHeight, coverDepth, 4, 0.2);
    const coverMaterials = [
      materials.box, materials.box, materials.box,
      materials.invisible, materials.box, materials.box,
    ];
    const cover = new THREE.Mesh(coverGeometry, coverMaterials);
    cover.scale.set(0.999, 0.999, 0.999);
    cover.castShadow = true;
    cover.position.y = coverBoxHeight / 2;
    coverGroup.add(cover);

    // SWM logo on box
    const logoTexture = new THREE.TextureLoader().load(swmLogo);
    logoTexture.colorSpace = THREE.SRGBColorSpace;
    
    const logoMaterialFront = new THREE.MeshBasicMaterial({
      map: logoTexture,
      transparent: true,
      side: THREE.FrontSide,
    });
    const logoMaterialBack = new THREE.MeshBasicMaterial({
      map: logoTexture,
      transparent: true,
      side: THREE.FrontSide,
    });
    
    const logoWidth = coverWidth * 0.7;
    const logoHeight = coverBoxHeight * 0.5;
    const logoPlaneGeometry = new THREE.PlaneGeometry(logoWidth, logoHeight);
    
    const logoFront = new THREE.Mesh(logoPlaneGeometry, logoMaterialFront);
    logoFront.position.set(0, coverBoxHeight / 2, coverDepth / 2 + 0.01);
    coverGroup.add(logoFront);
    
    const logoBack = new THREE.Mesh(logoPlaneGeometry, logoMaterialBack);
    logoBack.position.set(0, coverBoxHeight / 2, -coverDepth / 2 - 0.01);
    logoBack.rotation.y = Math.PI;
    coverGroup.add(logoBack);

    // White trim outline
    const outlineGeom = coverGeometry.clone();
    outlineGeom.scale(1.002, 1.002, 1.002);
    const threshold = 0.001;
    const edgesGeom = new THREE.EdgesGeometry(outlineGeom, threshold);
    const posAttr = edgesGeom.attributes.position;
    const positions: number[] = [];
    for (let i = 0; i < posAttr.count; i++) {
      positions.push(posAttr.getX(i), posAttr.getY(i), posAttr.getZ(i));
    }

    const segGeometry = new LineSegmentsGeometry();
    segGeometry.setPositions(positions);

    const lineMaterial = new LineMaterial({
      color: 'rgba(215, 207, 207, 0.65)',
      linewidth: 4,
      dashed: false,
      alphaToCoverage: false,
    });

    const size = new THREE.Vector2();
    renderer.getSize(size);
    lineMaterial.resolution.set(size.x, size.y);

    const thickLines = new LineSegments2(segGeometry, lineMaterial);
    thickLines.computeLineDistances();
    thickLines.renderOrder = 999;
    cover.add(thickLines);

    // String that pulls the box
    const MAX_STRING_LENGTH = C.ANCHOR_Y - C.COVER_TOP_Y;
    const stringGeometry = new THREE.CylinderGeometry(0.1, 0.1, MAX_STRING_LENGTH, 8);
    const string = new THREE.Mesh(stringGeometry, materials.body);
    coverGroup.add(string);

    return { coverGroup, string, lineMaterial };
  };

  const startAnimationLoop = (
    renderer: THREE.WebGLRenderer,
    scene: THREE.Scene,
    camera: THREE.PerspectiveCamera,
    controls: OrbitControls
  ) => {
    const animate = () => {
      requestAnimationFrame(animate);

      const sceneRefs = sceneRefsRef.current;
      const state = animationStateRef.current;

      if (sceneRefs?.recordGroup) {
        sceneRefs.recordGroup.rotation.y += C.RECORD_ROTATION_SPEED;
      }

      if (sceneRefs?.coverGroup && sceneRefs?.string) {
        let newCoverY: number;

        if (state.isSliderPressed) {
          const currentY = sceneRefs.coverGroup.position.y;
          newCoverY = currentY + (state.targetCoverY - currentY) * C.LERP_FACTOR_ACTIVE;
          sceneRefs.coverGroup.position.y = newCoverY;
          state.fallVelocity = 0;
        } else {
          const currentY = sceneRefs.coverGroup.position.y;
          if (currentY > 0) {
            state.fallVelocity += C.GRAVITY;
            newCoverY = currentY - state.fallVelocity;
            if (newCoverY < 0) {
              newCoverY = 0;
            }
            sceneRefs.coverGroup.position.y = newCoverY;
          } else {
            newCoverY = 0;
            state.fallVelocity = 0;
          }

          const newSliderValue = (newCoverY / C.COVER_HEIGHT_3D_MAX) * C.SLIDER_MAX;
          state.coverHeight = Math.max(0, newSliderValue);
          if (inputRef.current) {
            inputRef.current.value = String(state.coverHeight);
          }
        }

        if (filter && gain && context) {
          const clampedY = Math.max(0, Math.min(newCoverY, C.COVER_HEIGHT_3D_MAX));
          const ratio = clampedY / C.COVER_HEIGHT_3D_MAX;
          const currentTime = context.currentTime;
          filter.frequency.setTargetAtTime(
            C.FILTER_MIN_FREQ + ratio * (C.FILTER_MAX_FREQ - C.FILTER_MIN_FREQ),
            currentTime,
            C.AUDIO_TRANSITION_TIME
          );
          gain.gain.setTargetAtTime(
            ratio * C.MAX_GAIN,
            currentTime,
            C.AUDIO_TRANSITION_TIME
          );
        }

        const MAX_STRING_LENGTH = C.ANCHOR_Y - C.COVER_TOP_Y;
        
        const isOnGround = newCoverY < 0.01;
        if (isOnGround && !state.wasOnGround) {
          if (bufferSourceRef.current) {
            stopSound();
          }
        }
        state.wasOnGround = isOnGround;

        const newStringLength = C.ANCHOR_Y - (newCoverY + C.COVER_TOP_Y);
        sceneRefs.string.scale.y = newStringLength / MAX_STRING_LENGTH;
        sceneRefs.string.position.y = C.COVER_TOP_Y + newStringLength / 2;
      }

      controls.update();
      renderer.render(scene, camera);
    };

    animate();
  };

  useEffect(() => {
    const currentMount = mountRef.current;
    if (!currentMount) return;

    const { scene, camera, renderer, controls } = initializeScene(currentMount);
    initializeLighting(scene);
    
    const materials = createMaterials();
    const recordGroup = createVinylPlayer(scene, materials);
    const { coverGroup, string, lineMaterial } = createCoverBox(scene, renderer, materials);

    sceneRefsRef.current = { recordGroup, coverGroup, string };
    
    startAnimationLoop(renderer, scene, camera, controls);

    const handleResize = () => {
      if (!currentMount) return;
      camera.aspect = currentMount.clientWidth / currentMount.clientHeight;
      camera.updateProjectionMatrix();
      renderer.setSize(currentMount.clientWidth, currentMount.clientHeight);
      try {
        const size = new THREE.Vector2();
        renderer.getSize(size);
        lineMaterial.resolution.set(size.x, size.y);
      } catch (e) {
        console.warn('Error updating line material resolution on resize:', e);
      }
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
      materials.labelTexture.dispose();
      renderer.dispose();
      if (currentMount && currentMount.contains(renderer.domElement)) {
        currentMount.removeChild(renderer.domElement);
      }
    };
  }, []);

  return (
    <div className={styles.container}>
      <div ref={mountRef} className={styles.canvasContainer} />
      <div className={styles.sliderContainer}>
        <input
          ref={inputRef}
          type="range"
          min="0"
          max={C.SLIDER_MAX}
          step="0.01"
          defaultValue="0"
          onChange={handleSliderChange}
          onMouseDown={handleSliderPress}
          onMouseUp={handleSliderRelease}
          onTouchStart={handleSliderPress}
          onTouchEnd={handleSliderRelease}
          className={styles.slider}
        />
      </div>
    </div>
  );
}