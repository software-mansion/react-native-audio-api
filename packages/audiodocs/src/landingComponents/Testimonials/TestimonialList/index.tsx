import clsx from 'clsx';
import React, { useRef, useState, useEffect, useCallback } from 'react';

import TestimonialItem from '../TestimonialItem';
import testimonials from './testimonials';
import styles from './styles.module.css';
import { Testimonial } from './types';
import useDrag from './useDrag';

const TestimonialList = () => {
  const [activeIndex, setActiveIndex] = useState(0);
  const [currentOffset, setCurrentOffset] = useState(0);
  const testimonialListRef = useRef<HTMLDivElement>(null);

  const onSetActiveSlide = useCallback((index: number) => {
    let offsetX = 0;

    testimonials.forEach((_t, i) => {
      const testimonialContainer = document.querySelector<HTMLElement>(`.testimonialContainer-${i}`);

      if (testimonialContainer && i < index) {
        offsetX += testimonialContainer.offsetWidth;
      }
    });


    setActiveIndex(index);
    setCurrentOffset(offsetX);
  }, []);

  const renderTestimonial = useCallback((testimonial: Testimonial, index: number) => (
    <div key={index} className={`testimonialContainer-${index}`} style={{ padding: '20px' }}>
      <TestimonialItem
        key={index}
        author={testimonial.author}
        company={testimonial.company}
        image={testimonial.image}
        link={testimonial.link}
      >
        {testimonial.body}
      </TestimonialItem>
    </div>
  ), []);

  useEffect(() => {
    function handleResize() {
      if (!testimonialListRef.current) {
        return;
      }

      const { offsetWidth } = testimonialListRef.current;

      let offsetX = 0;


      testimonials.forEach((_t, index) => {
        const testimonialContainer = document.querySelector<HTMLElement>(`.testimonialContainer-${index}`);

        if (testimonialContainer) {
          testimonialContainer.style.width = `${offsetWidth}px`;
        }

        if (index < activeIndex) {
          offsetX += testimonialContainer ? testimonialContainer.offsetWidth : 0;
        }
      })

      setCurrentOffset(offsetX);

      const activeSlide = document.querySelector<HTMLElement>(`.testimonialContainer-${activeIndex}`);

      if (!activeSlide) {
        return;
      }

      testimonialListRef.current.style.height = `${activeSlide.offsetHeight + 50}px`;
    }


    handleResize();
    window.addEventListener('resize', handleResize);

    return () => {
      window.removeEventListener('resize', handleResize);
    };
  }, [activeIndex]);

  const dragHandlers = useDrag(currentOffset, setCurrentOffset, activeIndex, onSetActiveSlide);

  return (
    <div>
      <div ref={testimonialListRef} className={styles.testimonialList}>
        <div {...dragHandlers} style={{
            transform: `translateX(-${currentOffset}px)`,
            display: 'flex',
            flexDirection: 'row',
            position: 'absolute',
            alignItems: 'flex-start',
            justifyContent: 'flex-start',
            transition: 'transform 300ms ease-out',
          }}
        >
          {testimonials.map(renderTestimonial)}
        </div>
      </div>
      <div>
        <div className={styles.dotsContainer}>
          {testimonials.map((_, index) => (
            <button
              key={index}
              type="button"
              onClick={() => onSetActiveSlide(index)}
              className={styles.dotButton}
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
      </div>
    </div>
  );
};

export default TestimonialList;
