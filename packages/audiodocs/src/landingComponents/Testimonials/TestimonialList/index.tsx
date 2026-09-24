import clsx from 'clsx';
import React, {
  useCallback,
  useEffect,
  useLayoutEffect,
  useRef,
  useState,
} from 'react';

import Icon from '@site/src/ui/Icon';

import TestimonialItem from '../TestimonialItem';
import testimonials from './testimonials';
import styles from './styles.module.css';
import { Testimonial } from './types';
import useDrag from './useDrag';

const SLIDE_VERTICAL_ALLOWANCE = 50;
const SLIDE_TRANSITION = 'transform 300ms ease-out';

const loopedTestimonials = [
  testimonials[testimonials.length - 1],
  ...testimonials,
  testimonials[0],
];

const FIRST_REAL_SLIDE = 1;
const LAST_REAL_SLIDE = testimonials.length;
const LEADING_COPY = 0;
const TRAILING_COPY = testimonials.length + 1;

const wrapSlideIndex = (index: number) =>
  ((index % testimonials.length) + testimonials.length) % testimonials.length;

const TestimonialList = () => {
  const [trackIndex, setTrackIndex] = useState(FIRST_REAL_SLIDE);
  const [slideWidth, setSlideWidth] = useState(0);
  const [trackHeight, setTrackHeight] = useState(0);
  const [isJumpingToCopiedSlide, setIsJumpingToCopiedSlide] = useState(false);
  const viewportRef = useRef<HTMLDivElement>(null);
  const trackRef = useRef<HTMLDivElement>(null);

  const activeIndex = wrapSlideIndex(trackIndex - FIRST_REAL_SLIDE);

  const goToRelativeSlide = useCallback((step: number) => {
    setTrackIndex((current) => {
      const next = current + step;

      return next < LEADING_COPY || next > TRAILING_COPY ? current : next;
    });
  }, []);

  const goToPreviousSlide = useCallback(
    () => goToRelativeSlide(-1),
    [goToRelativeSlide]
  );

  const goToNextSlide = useCallback(
    () => goToRelativeSlide(1),
    [goToRelativeSlide]
  );

  const onSetActiveSlide = useCallback(
    (index: number) => setTrackIndex(index + FIRST_REAL_SLIDE),
    []
  );

  const onSlideSettled = useCallback(
    (event: React.TransitionEvent) => {
      const hasSlideMoved =
        event.target === event.currentTarget &&
        event.propertyName === 'transform';

      if (!hasSlideMoved) {
        return;
      }

      if (trackIndex === TRAILING_COPY) {
        setTrackIndex(FIRST_REAL_SLIDE);
        setIsJumpingToCopiedSlide(true);
      } else if (trackIndex === LEADING_COPY) {
        setTrackIndex(LAST_REAL_SLIDE);
        setIsJumpingToCopiedSlide(true);
      }
    },
    [trackIndex]
  );

  useEffect(() => {
    if (!isJumpingToCopiedSlide) {
      return;
    }

    let paintedFrame = 0;
    const jumpedFrame = requestAnimationFrame(() => {
      paintedFrame = requestAnimationFrame(() =>
        setIsJumpingToCopiedSlide(false)
      );
    });

    return () => {
      cancelAnimationFrame(jumpedFrame);
      cancelAnimationFrame(paintedFrame);
    };
  }, [isJumpingToCopiedSlide]);

  useLayoutEffect(() => {
    function measureSlideWidth() {
      if (viewportRef.current) {
        setSlideWidth(viewportRef.current.offsetWidth);
      }
    }

    measureSlideWidth();
    window.addEventListener('resize', measureSlideWidth);

    return () => window.removeEventListener('resize', measureSlideWidth);
  }, []);

  useEffect(() => {
    const track = trackRef.current;

    if (!track) {
      return;
    }

    const observer = new ResizeObserver(() => setTrackHeight(track.offsetHeight));
    observer.observe(track);

    return () => observer.disconnect();
  }, []);

  const { dragOffset, isDragging, dragHandlers } = useDrag(
    slideWidth,
    goToRelativeSlide
  );

  const renderTestimonial = useCallback(
    (testimonial: Testimonial, index: number) => (
      <div
        key={index}
        className={styles.testimonialSlide}
        style={{ width: slideWidth || undefined }}
        aria-hidden={index === LEADING_COPY || index === TRAILING_COPY}
      >
        <TestimonialItem
          author={testimonial.author}
          company={testimonial.company}
          image={testimonial.image}
          link={testimonial.link}
        >
          {testimonial.body}
        </TestimonialItem>
      </div>
    ),
    [slideWidth]
  );

  return (
    <div>
      <div
        ref={viewportRef}
        className={styles.testimonialList}
        style={{
          height: trackHeight
            ? trackHeight + SLIDE_VERTICAL_ALLOWANCE
            : undefined,
        }}
      >
        <div
          {...dragHandlers}
          ref={trackRef}
          className={styles.testimonialTrack}
          onTransitionEnd={onSlideSettled}
          style={{
            transform: `translateX(${dragOffset - trackIndex * slideWidth}px)`,
            transition:
              isDragging || isJumpingToCopiedSlide ? 'none' : SLIDE_TRANSITION,
          }}
        >
          {loopedTestimonials.map(renderTestimonial)}
        </div>
      </div>
      <div className={styles.controlsContainer}>
        <button
          type="button"
          className={clsx(styles.arrowButton, styles.previousArrow)}
          onClick={goToPreviousSlide}
          aria-label="Previous testimonial"
        >
          <Icon name="chevronDown" size={20} />
        </button>
        <div className={styles.dotsContainer}>
          {testimonials.map((_, index) => (
            <button
              key={index}
              type="button"
              onClick={() => onSetActiveSlide(index)}
              className={styles.dotButton}
              aria-label={`Go to testimonial ${index + 1}`}
              aria-current={index === activeIndex}
            >
              <span
                className={clsx(
                  styles.dot,
                  index === activeIndex && styles.activeDot
                )}
              />
            </button>
          ))}
        </div>
        <button
          type="button"
          className={clsx(styles.arrowButton, styles.nextArrow)}
          onClick={goToNextSlide}
          aria-label="Next testimonial"
        >
          <Icon name="chevronDown" size={20} />
        </button>
      </div>
    </div>
  );
};

export default TestimonialList;
